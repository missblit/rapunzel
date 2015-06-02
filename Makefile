CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -pthread
LDFLAGS = -lboost_system -lboost_coroutine -lstdc++
OBJS = asio_util.o fcgi_connection_manager.o fcgi_connection.o \
       fcgi_request.o util.o
.PHONY: clean build
# Default target
build: rapunzel.a
# Dependencies
asio_util.o: asio_util.h
fcgi_connection_manager.o: fcgi_connection_manager.h
fcgi_connection.o: fcgi_connection_manager.h fcgi_connection.h
fcgi_request.o: fcgi_request.h fcgi_connection.h
util.o: util.h
fcgi_connection_manager.h: fcgi_connection.h
fcgi_connection.h: fcgi_types.h fcgi_request.h util.h
# Rules
rapunzel.a: ${OBJS}
	ar rs $@ $^ 
clean:
	rm -f rapunzel.a ${OBJS}