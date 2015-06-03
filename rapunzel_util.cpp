#include "rapunzel_util.h"

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

std::string decode_querystring_field(const std::string &s) {
	std::string res;
	for(std::size_t i = 0; i < s.size(); i++) {
		if(   s[i] >= 'A' && s[i] <= 'Z'
		   || s[i] >= 'a' && s[i] <= 'z'
		   || s[i] >= '0' && s[i] <= '9'
		   || s[i] == '*' || s[i] == '-'
		   || s[i] == '.' || s[i] == '_')
		{
			res += s[i];
			continue;
		}
		if(s[i] == '+') {
			res += ' ';
			continue;
		}
		//not enough bytes for hex-string left
		//so just treat whatever's left as plain characters
		if(i+3 >= s.size()) {
			res += s[i];
			continue;
		}
		//determine if the next three characters
		//are hex encoded or not
		auto hexval = [](char c) -> int {
			if(c >= '0' && c <= '9')
				return c - '0';
			if(c >= 'A' && c <= 'F')
				return c - 'A' + 10;
			if(c >= 'a' && c <= 'f')
				return c = 'a' + 10;
			return -1;
		};
		bool is_hex =    s[i] == '%'
		              && hexval(s[i+1]) != -1
					  && hexval(s[i+2]) != -1;
		if(!is_hex) {
			res += s[i];
			continue;
		}
		uint8_t val = hexval(s[i+1])*16 + hexval(s[i+2]);
		res += val;
		i+=2;
	}
	return res;
}		
		
std::map<std::string,std::string> decode_querystring(const std::string &s) {
	/* split up input into fields based on `&` */
	std::stringstream ss(s);
	std::vector<std::string> fields;
	{
		std::string field;
		while(std::getline(ss, field, '&'))
			fields.push_back(field);
	}
	/* split up field into key and value based on `=` */
	std::vector<std::pair<std::string,std::string>> kv_pairs;
	for(const std::string &field : fields) {
		auto pos = field.find_first_of('=');
		if(pos == std::string::npos)
			kv_pairs.emplace_back(field, "");
		else {
			std::string key = field.substr(0,pos);
			std::string val = field.substr(pos+1);
			kv_pairs.emplace_back(key,val);
		}
	}
	
	/* URL decode the keys and values
	 * reversing the HTML5 form encoding algorithm
	 */
	std::map<std::string,std::string> res;
	for(const auto &p : kv_pairs) {
		std::string left  = decode_querystring_field(p.first);
		std::string right = decode_querystring_field(p.second);
		res[left] = right;
	}
	return res;
}