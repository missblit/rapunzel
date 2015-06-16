#include "fcgi_connection_manager.h"
#include "rapunzel_util.h"
#include <typeinfo>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <iostream>

namespace fcgi {
	
void connection_manager::handle_accept(const boost::system::error_code& error) {
    if(error) {
        throw std::runtime_error("accept callback failed");
    }
    /* construct new connection from the socket */
	auto conn_ptr = std::make_unique<connection>(std::move(sock), this);
	/* and add it to the list of open connections */
	conns.push_back(std::move(conn_ptr));
}

connection_manager::connection_manager()
: sock(io), acceptor(io), pending_connection(nullptr),
  conn_thread(&connection_manager::run, this) {}
  
void connection_manager::run() {
    try {
		/* accept connections on the FCGI socket LISTENSOCK_FILENO */
        acceptor.assign(socket_protocol(LISTENSOCK_FILENO), LISTENSOCK_FILENO);
        while(true) {
            auto fun = std::bind(&connection_manager::handle_accept, this,
                                 std::placeholders::_1);
            acceptor.async_accept(sock, fun);
            io.run();
            io.reset();
        }
    }
    catch (std::exception& e) {
		/* not really any good place to put the debug output at this point */
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
}

request connection_manager::get_request() {
	connection *conn;
	{
		/* block for pending connection */
		std::unique_lock<std::mutex> lock(pending_mutex);
		pending_cv.wait(lock, [this]()->bool{return pending_connection;});
		conn = pending_connection;
		pending_connection = nullptr;
	}
	return conn->publish();	
}

}