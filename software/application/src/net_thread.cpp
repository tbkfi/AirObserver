#include "context.hpp"
#include "sockets.hpp"
#include "net_thread.hpp"

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/select.h>


LOG_MODULE_REGISTER(transport_thread, LOG_LEVEL_DBG);


namespace sys {
namespace net {

static context net_ctx;
context& ctx (void)
{
    return net_ctx;
}

// thread should communicate with a server!
// send data, expect something in return
// say, send data from sensors
void thread ()
{
    auto& ctx_wifi = sys::ctx ().wifi_data ();
    auto& ctx_net = sys::net::ctx ();

    // wait for wifi connectivity.
    while (ctx_wifi.status != NET_EVENT_WIFI_CONNECT_RESULT)
    {
        k_sleep (K_MSEC (1000));  // sleep for a second
        LOG_INF ("waiting for wifi interface");
    }

    // switch if no server is defined
    while (ctx ().server == nullptr)
    {
        LOG_INF ("server is not defined in net_ctx.");
        k_sleep (K_MSEC (1000));
    }

    sys::transport::connection socket;
    int status = socket.tcp_open (ctx().server, ctx().port );
    while (status)
    {
        status = socket.tcp_open (ctx().server, ctx().port );
        k_sleep (K_MSEC(1000));
    }

    LOG_INF ("socket %d started for %s:%d", status, ctx().server, ctx().port);

    socket.ws_open (ctx().server, ctx().port);

    sys::static_string <32> buf;
    fd_set readfds;

    auto& sock = socket.local_context.sock;

// example of using select () from POSIX
#if 0
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        struct timeval timeout = {
            .tv_sec = 1,
            .tv_usec = 0,
        };
#endif
#if 0
        int ret = select(sock + 1, &readfds, NULL, NULL, &timeout);
        size_t bytes = 0;
        k_sleep (K_MSEC(50));

        if (ret > 0 && FD_ISSET(sock, &readfds)) {
            buf.clear ();

            // Data is available
            LOG_INF ("select: %d, reading the data ...", ret);
            bytes = socket.ws_recv_all (reinterpret_cast <uint8_t *>(buf.data()), buf.capacity());

            printf ("RECV bytes: %d\n", bytes);
            printf ("RECV: %s\n", (const char *)buf.c_str ());

        } else if (ret == 0) {
            // Timeout
        } else {
            // error
        }

        // try restarting the TCP connection
        if (!socket.local_context.tcp_open)
        {
            LOG_ERR ("tcp socket was closed %d. try re-open ..." , socket.local_context.sock);
            socket.ws_open (ctx().server, ctx().port );

            while (!socket.local_context.ws_open)
            {
                k_sleep (K_MSEC(1000));
                socket.tcp_open (ctx().server, ctx().port );
            }
        }

        k_sleep (K_MSEC(50));
#endif
    }
    sys::static_string <32> ws_payload ("{\"echo\":\"bravo\"}");
    socket.ws_send ((void *)ws_payload.c_str(), ws_payload.size());

    sys::static_string <64> resp;
    socket.ws_recv_all (resp.data(), resp.capacity ());

    printf ("RECV: %s\n", resp.c_str ());
}

} // net
} // sys
