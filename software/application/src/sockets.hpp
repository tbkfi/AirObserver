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
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/unistd.h>

#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/websocket.h>
/////////////////////////////////////////////////////////////////////////////////
#include "static_string.h"
// maybe ...
// #include <zephyr/net/tls_credentials.h>

static int setup_socket
(sa_family_t family, const char *server, int port,
int *sock, struct sockaddr *addr, socklen_t addr_len);

static int connect_socket
(sa_family_t family, const char *server, int port,
int *sock, struct sockaddr *addr, socklen_t addr_len);

void spawn_socket ();

namespace sys {
namespace tcp {
struct context {
// connection-specific context
    bool tcp_open;
    bool ws_open;
    int sock;
    int sock_ws;
    int port;
    static_string <64> server;
};
} // tcp
} // sys

// object for creating and managing sockets
class Connection {
public:
    // each connection has its own context
    sys::tcp::context local_context;

    // public method to view the data
    sys::tcp::context& ctx ();

    // open TCP socket, but do not send anything
    int tcp_open (const char *server, int port);
    int ws_upgrade ();

    // close TCP socket, gracefully
    void tcp_close ();

    int raw_send (void *user_data, size_t data_len)
    {
        if (local_context.sock == -1)
        {
            LOG_ERR("tried sending, but socket is not open.");
            return 0;
        }
       
        int send_len = send (local_context.sock, user_data, data_len, 0);
        return send_len;
    }

private:
    // man sockaddr 3
    // It's aligned so that a pointer to it can be cast as a pointer 
    // to other sockaddr_* structures and used to access its fields.
    sockaddr remote_addr {};
};

namespace sys {
namespace net {
    struct context {
        
    };
    context& ctx (void);
}   // net 
}   // sys

