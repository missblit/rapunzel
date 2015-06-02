#ifndef RAPUNZEL_FCGI_TYPES_H
#define RAPUNZEL_FCGI_TYPES_H

#include <arpa/inet.h>

constexpr int LISTENSOCK_FILENO = 0;
constexpr int KEEP_CONN = 1;

enum class TYPE : uint8_t {
	BEGIN_REQUEST     =  1,
	ABORT_REQUEST     =  2,
	END_REQUEST       =  3,
	PARAMS            =  4,
	STDIN             =  5,
	STDOUT            =  6,
	STDERR            =  7,
	DATA              =  8,
	GET_VALUES        =  9,
	GET_VALUES_RESULT = 10,
	UNKNOWN_TYPE      = 11,
	MAXTYPE           = 11
};

/** The header of a FCGI message
 *@todo: remove nonstandard attributes, write better serialization code */
struct __attribute__((__packed__))
record_header {
	/** FCGI version (should be 1) */
	uint8_t   version;
	/** Type of the message */
	TYPE      type;
	/** Request ID the record is talking about */
	uint16_t  requestId;
	/** content length of the body */
	uint16_t  contentLength;
	/** padding after the body */
	uint8_t   paddingLength;
	/** reserved for future use */
	uint8_t   reserved;
	
	record_header() : version(1), contentLength(0), paddingLength(0),
	                  reserved(0) {}
	/** convert record header from network order to host order */
	void ntoh() {
		requestId = ntohs(requestId);
		contentLength = ntohs(contentLength);
	}
	/** convert record header from host order to network order */
	void hton() {
		requestId = htons(requestId);
		contentLength = htons(contentLength);
	}
	
};

/** The body of an FCGI BEGIN_REQUEST message */
struct __attribute__((__packed__))
begin_request_body {
	/** The FCGI role */
	uint16_t role;
	uint8_t flags;
	uint8_t reserved[5];
	
	void ntoh() {
		role = ntohs(role);
	}
};

#endif