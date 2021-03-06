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
		if(!qs.empty()) {
			r << "<p>Here is a list of query string key-value pairs!\n"
				" but watch out for XSS, I'm not sanitizing them!<br>\n";
			for(auto &p : qs)
				r << "\"" << p.first << "\" -> \"" << p.second << "\"<br>\n";
			r << "</p>\n";
		}
	}
}
