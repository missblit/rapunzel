#ifndef RAPUNZEL_FCGI_REQUEST_H
#define RAPUNZEL_FCGI_REQUEST_H

#include <cstdint>
#include <map>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

namespace fcgi {
	
class connection;
class request;

/** std::streambuf interface to the request */
class request_streambuf : public std::streambuf {
private:
	request *r;
protected:
	virtual std::streamsize xsputn( const char_type* s, std::streamsize count );
public:
	request_streambuf(request *r);
};

class request : public std::ostream {
private:
	std::unique_ptr<request_streambuf> my_streambuf;
	
	/** The FCGI request ID for this request */
	uint16_t id;
	/** Whether or not to close the connection when this request is done */
	bool keep_conn;
	/** Pointer to the connection managing this request */
	connection *conn;
	/** Request parameters */
	std::map<std::string, std::string> params;
	/** standard input
	 * currently read 100% before request is pending
	 * might switch to streaming the stdin in the future */
	std::string stdin_buff;
	/** whether the request has been closed or not */
	bool is_closed;
	/** whether the output stream has been closed or not */
	bool stdout_open;
	
	/** incoming request needs to keep track of stdin and params */
	bool stdin_done;
	bool params_done;
	
	friend class connection;
public:
	request(const request &r) = delete;
	request &operator=(const request &r) = delete;
	/** Construct a closed request that corresponds to no real FCGI request */
	request();
	/**
	 * @param id the FCGI request ID
	 * @param keep_conn Whether to close the connection when this request is done
	 * @param conn The connection managing this request
	 */
	request(uint16_t id, bool keep_conn, connection *conn);	
	/** r will be marked closed, and it's data will be copied into this */
	request(request &&r);
	~request();
	request &operator=(request &&r);
	
	/** write on stdout */
	void write(std::string message);
	/** close stdout */
	void close_stdout();
	/** close stdout, then the connection */
	void close();
	/** Get the connections parameters */
	const std::map<std::string, std::string> &parameters() const;
	/** Get the connection's standard input as a string */
	const std::string &stdin() const;
};

} //namespace fcgi

#endif