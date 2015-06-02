#ifndef RAPUNZEL_FCGI_CONNECTION_MANAGER_H
#define RAPUNZEL_FCGI_CONNECTION_MANAGER_H

#include <thread>
#include <condition_variable>
#include "fcgi_connection.h"

namespace fcgi {
    /**
     * Class that sits between user code and all FCGI connections
     */
    class connection_manager {
    friend class connection;

	boost::asio::io_service io;
    /** The socket to listen for connections on */
    boost::asio::generic::stream_protocol::socket sock;
    boost::asio::basic_socket_acceptor
    <boost::asio::generic::stream_protocol> acceptor;
    
    /** Set of open connections */
    std::vector<std::unique_ptr<connection>> conns;	
    


    /** Only one connection can write at once, so writers need this mutex */
    std::mutex write_mutex;
    /** Used to listen for new connections from the read thread */
    std::condition_variable pending_cv;
    std::mutex pending_mutex;
    connection *pending_connection;

    /** The thread that listens for new connections */
    std::thread conn_thread;

	/** Called by the read thread to make a new request pending */
    void publish(connection *conn);
	/** boost::asio callback to handle new connections */
    void handle_accept(const boost::system::error_code& error);

	/** Called on conn_thread to listen for new connections */
    void run();

public:
	/** Construct and start listening for new connections on the FCGI socket */
	connection_manager();
    /** Block until there is a pending request, and then return it */
    request get_request();
};

}

#endif