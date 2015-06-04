CXXFLAGS = -std=c++14 -Wall -Wextra -pthread -g -O0
LDFLAGS = -lboost_system -lboost_coroutine -lstdc++
OBJS = rapunzel_util.o fcgi_connection_manager.o fcgi_connection.o \
       fcgi_request.o util.o
.PHONY: clean build
# Default target
build: rapunzel.a
# Dependencies
rapunzel_util.o: rapunzel_util.h
fcgi_connection_manager.o: fcgi_connection_manager.h
fcgi_connection.o: fcgi_connection_manager.h fcgi_connection.h
fcgi_request.o: fcgi_request.h fcgi_connection.h
util.o: util.h
fcgi_connection_manager.h: fcgi_connection.h
fcgi_connection.h: fcgi_types.h fcgi_request.h util.h
util.h: rapunzel_util.h
# Rules
rapunzel.a: ${OBJS}
	ar rs $@ $^ 
clean:
	rm -f rapunzel.a ${OBJS}
main: rapunzel.a
	${CXX} ${CXXFLAGS} ${LDFLAGS} main.cpp rapunzel.a
