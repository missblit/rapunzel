#ifndef ASIO_UTIL_H
#define ASIO_UTIL_H

#include <string>
#include <map>
#include <boost/asio.hpp>

/**
 * Construct a generic stream_protocol from a posix socket.
 * The socket can be IPv4, IPv6, or a unix socket
 */
boost::asio::generic::stream_protocol socket_protocol(int sock);

/**
 * Turn an arbitrary object into an asio buffor
 * note: this will not get rid of object padding, or deal with endianness
 * @param p The Plain-Old-Data to buffer
 */
template <typename POD>
auto make_asio_buffer(POD &p) { return boost::asio::buffer(&p, sizeof(p)); }

/** HTML form decode a strung */
std::string decode_querystring_field(const std::string &s);

/** Takes a query string, breaks it into fields, URL decodes those fields
 * and returns the result as a map
 */
std::map<std::string,std::string> decode_querystring(const std::string &s);

#endif