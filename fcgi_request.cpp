#include <thread>
#include "fcgi_request.h"
#include "fcgi_connection.h"

static std::mutex mmmm;

namespace fcgi {
	
request::request()
: id(0), keep_conn(1), conn(nullptr), is_closed(true), stdout_open(false)
{
}

request::request(uint16_t id, bool keep_conn, connection *conn)
: id(id), my_streambuf(std::make_unique<request_streambuf>(this)),
  keep_conn(keep_conn), conn(conn), 
  is_closed(!conn), stdout_open(conn) {
	  rdbuf(my_streambuf.get());

};

request::request(request &&r)
: id(r.id), my_streambuf(std::move(r.my_streambuf)),
  keep_conn(r.keep_conn), conn(r.conn),
 params(std::move(r.params)),
  is_closed(r.is_closed), stdout_open(r.stdout_open) {
	if(my_streambuf)
		my_streambuf->r = this;
	r.rdbuf(nullptr);
	rdbuf(my_streambuf.get());
	
	r.stdout_open = false;
	r.is_closed = true;
	r.conn = nullptr;
}
 
request &request::operator=(request &&r) {
	my_streambuf = std::move(r.my_streambuf);
	if(my_streambuf)
		my_streambuf->r = this;
	rdbuf(my_streambuf.get());
	r.rdbuf(nullptr);
	
	id = r.id;
	keep_conn = r.keep_conn;
	conn = r.conn;
	is_closed = r.is_closed;
	stdout_open = r.stdout_open;
	params = std::move(r.params);
	
	r.stdout_open = false;
	r.is_closed = true;
	r.conn = nullptr;

	return *this;
}


void request::write(std::string message) {
	/* harmless to output an empty string, but if it was a FCGI message it would
	 * close the stream */
	if(message == "")
		return;
	conn->write(id, message);
}

void request::close_stdout() {
	if(is_closed || !stdout_open)
		return;
	stdout_open = false;
	my_streambuf.reset();
	conn->write(id, "");
}

void request::close() {
	if(is_closed)
		return;
	close_stdout();
	conn->close(id);
	is_closed = true;
	
}

request::~request() {
	close();
}

request_streambuf::request_streambuf(request *r) : r(r) {}	
	
std::streamsize request_streambuf::xsputn(const char_type* s,
                                          std::streamsize count )
{
	std::string str(s, s+count);
	r->write(str);
	return count;
}


const std::map<std::string, std::string> &request::parameters() const {
	return params;
}

const std::string &request::stdin() const {
	return stdin_buff;
}

} //namespace fcgi