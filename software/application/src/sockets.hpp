#pragma once

#include <zephyr/kernel.h>
#include <stdint.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>
#include <zephyr/net/http/client.h>

#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/arpa/inet.h>

#include <zephyr/logging/log.h>

////////////////////////////////////////////////////////////////////////////////
#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/websocket.h>
/////////////////////////////////////////////////////////////////////////////////
#include "static_string.h"
// maybe ...
// #include <zephyr/net/tls_credentials.h>


// static int setup_socket
// (sa_family_t family, const char *server, int port,
// int *sock, struct sockaddr *addr, socklen_t addr_len);
// 
// static int connect_socket
// (sa_family_t family, const char *server, int port,
// int *sock, struct sockaddr *addr, socklen_t addr_len);
// 
// void spawn_socket ();

namespace sys {
namespace transport {
struct context {
// connection-specific context
    bool tcp_open;
    bool ws_open;
    int sock;
    int sock_ws;
    int port;
    sys::static_string <64> server;
};
// object for creating and managing sockets
class connection {
public:
    // each connection has its own context
    sys::transport::context local_context {};

    // public method to view the data
    sys::transport::context& ctx ();

    // open TCP socket, but do not send anything
    int tcp_open (const char *server, int port);
    int ws_open (const char *server, int port);

    // close TCP socket, gracefully
    void tcp_close ();
    void ws_close ();
    
    // send raw data in TCP socket
    int tcp_send (void *, size_t);
    int tcp_recv (size_t, uint8_t *, size_t );
    int tcp_recv_all (uint8_t *, size_t);

    ssize_t ws_send (void *, size_t);
    void ws_recv (size_t, uint8_t *, size_t);
    int ws_recv_all (uint8_t *, size_t);

private:
    // man sockaddr 3
    // It's aligned so that a pointer to it can be cast as a pointer 
    // to other sockaddr_* structures and used to access its fields.
    net_sockaddr_in remote_addr {};
};

} // transport
} // sys

