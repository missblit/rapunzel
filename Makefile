CXX = g++
CXXFLAGS = -std=c++14 -Wall -pthread -Wfatal-errors
LDFLAGS = -lboost_system -lboost_coroutine -lstdc++

OBJS = asio_util.o fcgi_connection_manager.o fcgi_connection.o \
       fcgi_request.o util.o
asio_util.cpp: asio_util.h
fcgi_connection_manager.cpp: fcgi_connection_manager.h

fcgi_connection.cpp: fcgi_connection_manager.h fcgi_connection.h
fcgi_request.cpp: fcgi_request.h fcgi_connection.h
util.cpp: util.h
fcgi_connection_manager.h: fcgi_connection.h
fcgi_connection.h: fcgi_types.h fcgi_request.h util.h
rapunzel.a: ${OBJS}
	ar rs $@ $^ 
deploy: main
	sudo touch /tmp/rapunzel-output
	sudo rm /tmp/rapunzel-output
	sudo systemctl stop lighttpd
	sudo cp main /usr/local/bin/hello
	sudo systemctl start lighttpd
	sleep 1
	systemctl status lighttpd -l

clean:
	rm -f main *.o
	
kill:
	sudo killall -s 9 hello || true 
	sudo systemctl stop lighttpd || true
