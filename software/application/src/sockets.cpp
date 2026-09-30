
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/websocket.h>

#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/arpa/inet.h>

#include <zephyr/logging/log.h>

//////////////////////////////////////////////////////////////////////////////
// zaebal rugat'sya
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/unistd.h>

#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/websocket.h>
/////////////////////////////////////////////////////////////////////////////

LOG_MODULE_REGISTER(net_http_client_sample, LOG_LEVEL_DBG);

#include "sockets.hpp"
#include "static_string.h"

// open TCP socket, but do not send anything
int Connection::tcp_open (const char *server, int port)
{
    int ret = 0;

    local_context.server = server;
    local_context.port = port;

    socklen_t addr_len = sizeof (remote_addr);
    memset(&remote_addr, 0, addr_len);

    // AF_INET == ipv4, AF_INET6 == ipv6.
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);  // converts to correct endianness

    // convert address to binary form
    inet_pton(remote_addr.sin_family, server, &remote_addr.sin_addr);

    const char *family_str = remote_addr.sin_family == AF_INET ? "IPv4" : "IPv6";
    LOG_INF ("connecting to %s:%s ...", family_str, server);

    local_context.sock = socket(remote_addr.sin_family, SOCK_STREAM, IPPROTO_TCP);
    if (local_context.sock < 0)
    {
        LOG_ERR ("failed to open %s socket (%d)", family_str, -errno);
    }

    ret = connect(local_context.sock, &remote_addr, sizeof (remote_addr));

    if (ret < 0) {
        LOG_ERR("cannot connect to %s remote (%d)", family_str, -errno);

        close(local_context.sock);
        local_context.sock = -1;
        ret = -errno;
    }

    LOG_ERR("socket open [%d] for %s (%d)", local_context.sock, server, -errno);
    return ret;
}

// TODO: WS upgrade
int Connection::ws_upgrade ()
{
    const char *extra_headers[] = {"Origin: http://foobar\r\n", NULL };
    websocket_request req {};

    req.host = local_context.server.c_str ();
    req.url = "/";
    req.optional_headers = extra_headers;
    req.cb = nullptr ; // TODO:
    req.tmp_buf = nullptr ; // TODO;
    req.tmp_buf_len = 0; // TODO:

    local_context.sock_ws = websocket_connect(local_context.sock, &req, 100, (void *)"IPv4");

    if (local_context.sock_ws < 0) {
        LOG_ERR("Cannot connect to %s:%d", local_context.server, local_context.port);
        close(local_context.sock);
    }
}

void Connection::tcp_close ()
{
    close (local_context.sock);
    local_context.sock = -1;
}

