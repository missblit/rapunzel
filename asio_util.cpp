#include "asio_util.h"

/* A fastCGI program is launched with STDIN being either a UNIX socket (AF_UNIX)
 * or a tcp/ip socket (AF_INET). This "just works" when using POSIX functions,
 * but boost.asio wants to know the socket protocol.
 * 
 * Instead of lying to it, I wrote this function to get the protocol out of the
 * socket */
boost::asio::generic::stream_protocol socket_protocol(int sock) {
	sockaddr  addr; 
	socklen_t addrlen = sizeof(addr);
	int res = getsockname(sock, &addr, &addrlen);
	if(res != 0) {
		//strerror isn't thread-safe, but IDGAF
		throw std::runtime_error(  "getsockname(): " 
		                         + std::string(strerror(errno)));
	}
	auto family = addr.sa_family;
	switch(family) {
		case AF_INET:  return boost::asio::ip::tcp::v4();
		case AF_INET6: return boost::asio::ip::tcp::v6();
		case AF_UNIX:  return boost::asio::local::stream_protocol();
		default: 
			throw std::runtime_error(  std::string("Unsupported addr family: ")
			                         + std::to_string(family));
	}
}