#include <array>
#include <thread>
#include <functional>
#include "fcgi_connection.h"
#include "fcgi_connection_manager.h"

namespace fcgi {

connection::connection(boost::asio::generic::stream_protocol::socket &&sock,
					   connection_manager *manager)
: sock(std::move(sock)), manager(manager), strand_(manager->io)  {
	start();
}

void connection::start() {
    boost::asio::spawn(io(), std::bind(&connection::read_header,
                                       this, std::placeholders::_1));
}

template<> void connection::type_handler<TYPE::ABORT_REQUEST>
(boost::asio::yield_context, record_header){
	throw std::runtime_error("Unhandled Type ABORT");
};

template<> void connection::type_handler<TYPE::END_REQUEST>
(boost::asio::yield_context, record_header){
	throw std::runtime_error("Unhandled Type END");
};

template<> void connection::type_handler<TYPE::STDIN>
(boost::asio::yield_context yield, record_header r) {
    /* read the message contents into a vector and turn it into a string*/
    std::vector<uint8_t> v(r.contentLength + r.paddingLength);
    async_read(sock, boost::asio::buffer(v), yield);
	std::string s(begin(v),end(v));
	
	/* if the string isn't empty, append it to stdin */
	if(!s.empty())
		incoming[r.requestId].stdin_buff += s;
	/* if the string is empty, then stdin is finished */
	else
		incoming[r.requestId].stdin_done = true;
	/* if both stdin and params are done, then make the request pending
	 * fixme: remove duplicate code w/ params */
	if(incoming[r.requestId].stdin_done && incoming[r.requestId].params_done)
		make_pending(r.requestId);
    start();
};

/** This message type should never be received */
template<> void connection::type_handler<TYPE::STDOUT>
(boost::asio::yield_context, record_header){
	throw std::runtime_error("Unhandled Type STDOUT");
};

/** This message type should never be received */
template<> void connection::type_handler<TYPE::STDERR>
(boost::asio::yield_context, record_header){
	throw std::runtime_error("Unhandled Type STDERR");
};

/** Stub for DATA */
template<> void connection::type_handler<TYPE::DATA>
(boost::asio::yield_context yield, record_header r){
    /* read the message contents into a vector and turn it into a string*/
    std::vector<uint8_t> v(r.contentLength + r.paddingLength);
    async_read(sock, boost::asio::buffer(v), yield);
	std::string s(begin(v),end(v));
	
	start();
};

/** stub fore GET_VALUES */
template<> void connection::type_handler<TYPE::GET_VALUES>
(boost::asio::yield_context yield, record_header r){
	/* read the message contents into a vector and discard it*/
    std::vector<uint8_t> v(r.contentLength + r.paddingLength);
    async_read(sock, boost::asio::buffer(v), yield);
	
	/* return an empty set of values */
	record_header rec;
	rec.requestId = 0;
	rec.type = TYPE::GET_VALUES_RESULT;
	rec.contentLength = 0;
	rec.hton();
	{
		std::lock_guard<std::mutex> lock(manager->write_mutex);
		boost::asio::write(sock, make_asio_buffer(rec));
	}
	start();
};

/** This message type should never be received */
template<> void connection::type_handler<TYPE::GET_VALUES_RESULT>
(boost::asio::yield_context, record_header){
	throw std::runtime_error("Unhandled Type GET VALUES RESULT");
};

template <>
void connection::type_handler<TYPE::BEGIN_REQUEST>
(boost::asio::yield_context yield, record_header r)
{
	begin_request_body b;
	async_read(sock, make_asio_buffer(b), yield);
	b.ntoh();
    bool keep_conn = b.flags & KEEP_CONN;
    /* add the request that began as a new incoming request */
	incoming[r.requestId] = {r.requestId, keep_conn, this};
	start();
}


void connection::make_pending(uint16_t id) {
	auto it = incoming.find(id);
	request r = std::move(it->second);
	incoming.erase(it);
	{
		std::lock_guard<std::mutex> lock(req_mutex);
		pending[id] = std::move(r);
	}
	std::lock_guard<std::mutex> manager_lock(manager->pending_mutex);
	manager->pending_connection = this;
	/* send a message when a new pending request is ready */
	manager->pending_cv.notify_one();
}

request connection::publish() {
	std::lock_guard<std::mutex> lock(req_mutex);
	auto it = pending.begin();
	request r = std::move(it->second);
	pending.erase(it);
	return r;
}

template <>
void connection::type_handler<TYPE::PARAMS>
(boost::asio::yield_context yield, record_header r) {
	std::vector<std::pair<std::string,std::string>> params;
	/* read all the parameter bytes */
	uint16_t bytes_pending = r.contentLength;
	while(bytes_pending) {
		/* this code is needed becahse FCGI uses a variable byte length for the
		 * length field
		 * see FCGI spec */
		uint8_t length_b0;
		uint32_t length, name_length, value_length;		
		std::array<std::reference_wrapper<uint32_t>, 2> lengths = {name_length,
			                                                       value_length};
		for(uint32_t &target : lengths) {
			auto b0_buff = boost::asio::buffer(&length_b0, 1);
			async_read(sock, b0_buff, yield);	
			if(length_b0 < 128) {
				target = length_b0;
				bytes_pending -= 1;
			}
			else {
				async_read(sock, boost::asio::buffer(&length, 3), yield);
				target = length_b0 << 24 | length;
				bytes_pending -= 4;
			}
		}
		/* read the name value pair */
		std::vector<uint8_t> name(name_length), value(value_length);
		std::vector<boost::asio::mutable_buffer> buffers;
		buffers.push_back(boost::asio::buffer(name));
		buffers.push_back(boost::asio::buffer(value));
		async_read(sock, buffers, yield);
		/* inser the name value pair */
		std::string  name_str(begin(name),  end(name)),
	                value_str(begin(value), end(value));
		params.emplace_back(name_str, value_str);
		
		bytes_pending -= (name_length + value_length);
	}
	/* if there were params, inser them */
	if(!params.empty())
		incoming[r.requestId].params.insert(begin(params), end(params));
	/* otherwise there are no more params */
	else
		incoming[r.requestId].params_done = true;
	/* if there were no more params, and stdin is done too, 
	 * then make the request pending */
	if(incoming[r.requestId].stdin_done && incoming[r.requestId].params_done)
		make_pending(r.requestId);
	start();
}

void connection::read_header(boost::asio::yield_context yield) {
	/* function table for doing different stuff depending on record type */
	std::array<
		void (connection::*)(boost::asio::yield_context,record_header), 10
	> funcs =
	{
		&connection::type_handler<TYPE::BEGIN_REQUEST>,
		&connection::type_handler<TYPE::ABORT_REQUEST>,
		&connection::type_handler<TYPE::END_REQUEST>,
		&connection::type_handler<TYPE::PARAMS>,
		&connection::type_handler<TYPE::STDIN>,
		&connection::type_handler<TYPE::STDOUT>,
		&connection::type_handler<TYPE::STDERR>,
		&connection::type_handler<TYPE::DATA>,
		&connection::type_handler<TYPE::GET_VALUES>,
		&connection::type_handler<TYPE::GET_VALUES_RESULT>
	};
	
	record_header r;
    boost::system::error_code ec;
	async_read(sock, boost::asio::buffer(&r, sizeof(r)), yield[ec]);
    if(ec.value()) {
        sock.close();
        return;
    }
	r.ntoh();
	
	const int type_int = static_cast<int>(r.type);

	if(type_int > 10)
		throw std::runtime_error("Unrecognized header type");
	else {
		/* call the appropriate handler */
		boost::asio::spawn(io(), std::bind(funcs[type_int-1], this,
		                                   std::placeholders::_1, r));	
	}
}

boost::asio::io_service &connection::io() {
	return manager->io;
}

void connection::write(uint16_t id, std::string message) {
    const char *data = message.c_str();
    auto remaining = message.size();
    std::lock_guard<std::mutex> lock(manager->write_mutex);
    //write the message in blocks of 65535 (the maximum length in the protocol)
    //do-while instead of while so that zero length messages will get through
    do {
        auto message_size = std::min(static_cast<std::size_t>(65535),
                                     remaining);
        remaining -= message_size;
        
        record_header r;
        r.requestId = id;
        r.type = TYPE::STDOUT;
        r.contentLength = message_size;
        r.hton();
        
        boost::asio::write(sock, make_asio_buffer(r));
        //If the size is 0 then the following is unneeded
        if(message.size()) {
            auto buff = boost::asio::buffer(data, message_size);
            boost::asio::write(sock, buff);
            data += message_size;
        }
    } while(remaining);
}

void connection::close(uint16_t id) {
	record_header r;
	r.requestId = id;
	r.type = TYPE::END_REQUEST;
	r.hton();
	std::lock_guard<std::mutex> lock(manager->write_mutex);
	boost::asio::write(sock, make_asio_buffer(r));
}

}
