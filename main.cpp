#include <random>
#include <map>
#include <string>
#include <regex>
#include "fcgi_connection_manager.h"
#include "rapunzel_util.h"

int main() {
	fcgi::connection_manager fcgi;
	unsigned counter = 0;
	while(true) {
		fcgi::request r = fcgi.get_request();
		auto qs = decode_querystring(r.parameter("QUERY_STRING"));
        r << "Content-type: text/html\r\n\r\n"
		     "<!DOCTYPE html>\n"
		     "<p>Hello, visitor number " << ++counter << "</p>\n";
		for(auto &p : qs) {
			r << "\"" << p.first << "\" -> \"" << p.second << "\"<br>\n";
		}
    }
}
