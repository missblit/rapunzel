#include <random>
#include <map>
#include <string>
#include <regex>
#include "fcgi_connection_manager.h"

std::map<std::string,std::string> map_query_string(fcgi::request &r) {
	auto qs_it = r.parameters().find("QUERY_STRING");
	std::string qs = (qs_it != r.parameters().end()) ? qs_it->second : "";

	std::map<std::string,std::string> qs_map;
	auto pos = qs.begin();
	while(pos != qs.end()) {
		//r << "arr\n";
		auto equ_pos = std::find(pos, qs.end(), '=');
		auto and_pos = std::find(pos, qs.end(), '&');
		if(equ_pos == qs.end() || equ_pos > and_pos) {
			qs_map[std::string(pos,and_pos)] = "";
			pos = and_pos;
		}
		else {
			auto lhs = std::string(pos, equ_pos);
			auto rhs = std::string(equ_pos+1, and_pos);
			qs_map[lhs] = rhs;
			pos = and_pos;
		}
		if(pos == qs.end())
			return qs_map;
        pos++;
	}
	return qs_map;
}

int main() {
	fcgi::connection_manager fcgi;
	while(true) {
		fcgi::request r = fcgi.get_request();
		const auto param_count = std::to_string(r.parameters().size());
        const auto &qs_map = map_query_string(r);
        r << "Content-type: text/html\r\n\r\n"
		     "<!DOCTYPE html>\n"
		     "<p>Hello, World!</p>\n";
    }
}
