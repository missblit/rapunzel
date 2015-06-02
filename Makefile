CXX = g++
CXXFLAGS = -std=c++14 -Wall -pthread -Wfatal-errors
LDFLAGS = -lboost_system -lboost_coroutine -lstdc++
.PHONY: deploy
#TODO - dependencies are all f-d up
default:
	make clean
	make deploy
main: fcgi_connection.o fcgi_connection_manager.o asio_util.o util.o \
      fcgi_request.o

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
