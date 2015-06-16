#ifndef RAPUNZEL_FCGI_CONNECTION_H
#define RAPUNZEL_FCGI_CONNECTION_H

#include <string>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include "fcgi_types.h"
#include "fcgi_request.h"

namespace fcgi {

class connection_manager;

class connection {
	/** The socket that this connection class wraps */
	boost::asio::generic::stream_protocol::socket sock;
	/** The connection_manager that this connection is managed by */
	connection_manager *manager;
	
	boost::asio::io_service::strand strand_;
	
	/** Requests that are in the process of being received
	 * note: no mutex needed because user code doesn't access this
	 */
	std::map<uint16_t, request> incoming;

	/** mutex for the pending requests */ 
	std::mutex req_mutex;
	/** requests that are ready to be accepted */
	std::map<uint16_t, request> pending;
	
	//std::map<uint16_t, std::weak_ptr<request>> open;

	/** boost asio callback scratch space
	 * (maybe could be replaced with coroutines?)
	 */
	struct asio_buffer {
		record_header      record;
		begin_request_body begin_request;
		uint8_t            name_length_b0;
		uint8_t            value_length_b0;
		uint32_t           name_length;
		uint32_t           vlaue_length;
	} asio_buff;
	
	/** start reading headers in a loop, by spawning read_header */
	void start();
	
	/** Different functions to handle different types of messages*/
	template <TYPE>
	void type_handler(boost::asio::yield_context yield, record_header);

	/**
	 * read a header and spawn the appropriate callback depending on it's type
	 */
	void read_header(boost::asio::yield_context yield);
	/** read a name/value pair */
	void read_nameval(boost::asio::yield_context yield);
	/** make the request go from incoming to pending
	 * (when it's ready for user code) */
	void make_pending(uint16_t id);
	/** return a pending request to the user
	 * removing it from the pending list
	 */
	request publish();

	/** Write a message using the given request ID
	 * @param id The request ID to use
	 * @param message The string to write
	 */
	void write(uint16_t id, std::string message);
	/** Tell the webserver to close the connection with the given request ID
	 * @param id the request to close
	 */
	void close(uint16_t id);
	/** retrieve the io_service associated iwth the connection_manager */
	boost::asio::io_service &io();
	
	friend class request;
	friend class connection_manager;
public:
	/** Turn the given socket into a new connection with the given
	 * connection_manager as this connection's manager
	 */
	connection(boost::asio::generic::stream_protocol::socket &&,
	           connection_manager *);
};

}

#endif