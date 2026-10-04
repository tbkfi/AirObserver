
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/websocket.h>

#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/sys/select.h>

#include <zephyr/logging/log.h>

//////////////////////////////////////////////////////////////////////////////
// zaebal rugat'sya
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/websocket.h>
/////////////////////////////////////////////////////////////////////////////

#include "sockets.hpp"
#include "static_string.h"

#include "context.hpp"

LOG_MODULE_REGISTER(tcp_sockets, LOG_LEVEL_DBG);


static int connect_cb (int sock, struct http_request *req, void *user_data)
{
	LOG_INF("websocket %d for %s connected.", sock, (char *)user_data);
	return 0;
}

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
    int pton_ret = inet_pton(remote_addr.sin_family, server, &remote_addr.sin_addr);
    if (pton_ret != 1)
    {
        LOG_ERR ("invalid IPv4 address: %s", server);
        return -EINVAL;
    }

    const char *family_str = remote_addr.sin_family == AF_INET ? "IPv4" : "IPv6";
    LOG_INF ("connecting to %s:%s[%d] ...", family_str, server, port);

    local_context.sock = socket (remote_addr.sin_family, SOCK_STREAM, IPPROTO_TCP);
    if (local_context.sock < 0)
    {
        LOG_ERR ("failed to open %s socket (%d)", family_str, -errno);
        return local_context.sock;
    }

    ret = connect (local_context.sock, (struct sockaddr *) &remote_addr, sizeof (remote_addr));

    if (ret < 0) {
        LOG_ERR ("cannot connect to %s remote (%d): "
                 "%s", family_str, -errno, strerror(errno));

        close(local_context.sock);
        local_context.sock = -1;
        ret = -errno;
        return ret;
    }

    LOG_INF ("socket open [%d] for %s (%d)", local_context.sock, server, -errno);
    local_context.tcp_open = true;
    return ret;
}

// TODO: WS upgrade
int Connection::ws_upgrade ()
{
    static uint8_t tmp_buf [512] = {};
    const char *extra_headers[] = {"Origin: http://foobar\r\n", NULL };
    websocket_request req {};

    memcpy ((void *)req.host, local_context.server.c_str(), local_context.server.size());
    // req.host = local_context.server.c_str ();
    req.url = "/";
    req.optional_headers = extra_headers;
    req.cb = connect_cb;
    req.tmp_buf = tmp_buf;
    req.tmp_buf_len = sizeof tmp_buf;

    local_context.sock_ws = websocket_connect(local_context.sock, &req, 100, (void *)"IPv4");

    if (local_context.sock_ws < 0) {
        LOG_ERR("Cannot connect to %s:%d", local_context.server.c_str(), local_context.port);
        close(local_context.sock);
        return local_context.sock;
    }

    local_context.ws_open = true;
    return local_context.sock_ws;
}

void Connection::ws_recv
(size_t amount, uint8_t *buf, size_t buf_len)
{
    const char proto[] = "IPv4";

	uint64_t remaining = -1; // wraps to UULONG_MAX
	int total_read;
	uint32_t message_type;
	int ret, read_pos;

	read_pos = 0;
	total_read = 0;

	while (remaining > 0) {
		ret = websocket_recv_msg (
            local_context.sock_ws,
            buf + read_pos, 
            buf_len - read_pos,
            &message_type,
            &remaining, 0
        );

		if (ret < 0) {
			if (ret == -EAGAIN) {
				k_sleep(K_MSEC(50));
				continue;
			}

			LOG_DBG("%s connection closed while "
				"waiting (%d/%d)", proto, ret, errno);
			break;
		}

		read_pos += ret;
		total_read += ret;
	}

	if (remaining != 0)
    {
		LOG_ERR("%s data recv failure %zd/%d bytes (remaining %d)",
			proto, amount, read_pos, remaining);
	} else {
		LOG_DBG("%s recv %d bytes", proto, total_read);
	}
}

int Connection::ws_recv_all (uint8_t *buf, size_t buf_len)
{
    const char proto[] = "IPv4";

	uint64_t remaining = -1; // wraps to UULONG_MAX
	int total_read;
	uint32_t message_type;
	int ret, read_pos;

	read_pos = 0;
	total_read = 0;

	while (remaining > 0) {
		ret = websocket_recv_msg (
            local_context.sock_ws,
            buf + read_pos, 
            buf_len - read_pos,
            &message_type,
            &remaining, 0
        );

		if (ret < 0) {
			if (ret == -EAGAIN) {
				k_sleep(K_MSEC(50));
				continue;
			}

			LOG_DBG("%s connection closed while "
				"waiting (%d/%d)", proto, ret, errno);
			break;
		}

		read_pos += ret;
		total_read += ret;
	}

    LOG_DBG("%s recv %d bytes", proto, total_read);
    return total_read;
}
// int Connection::ws_recv (uint8_t *buf, size_t len, )

int Connection::tcp_recv
(size_t amount, uint8_t *buf, size_t buf_len)
{
    const char proto[] = "IPv4";

	int remaining;
	int ret, read_pos;

	remaining = amount;
	read_pos = 0;

	while (remaining > 0) {
		ret = recv(local_context.sock, buf + read_pos, buf_len - read_pos, 0);
		if (ret < 0) {
			if (errno == EAGAIN || errno == ETIMEDOUT) {
				k_sleep(K_MSEC(50));
				continue;
			}
		}

        if (ret == 0)
        {
			LOG_DBG("connection closed while "
				    "waiting (%d/%d)", ret, errno);
			break;
        }

		read_pos += ret;
		remaining -= ret;
	}

	if (remaining != 0 ) {
		LOG_ERR("data recv failure %zd/%d bytes (remaining %d)",
			amount, read_pos, remaining);
	} else {
		LOG_DBG("%s recv %d bytes", proto, read_pos);
	}

    return read_pos;
}

int Connection::tcp_recv_all (uint8_t *buf, size_t buf_len) // DONE
{
    const char proto[] = "IPv4";
	int remaining;
	int ret, read_pos;

	remaining = buf_len;
	read_pos = 0;

	while (remaining > 0) {
		ret = recv(local_context.sock, buf + read_pos, buf_len - read_pos, MSG_DONTWAIT);
		if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // no data to read, do not block
                break;
			}

            if (errno == ECONNRESET)
            {
			    LOG_DBG("connection closed while waiting (%d/%d)", ret, errno);
                tcp_close ();
                break;
            }

			LOG_DBG("unhandled error (%d/%d)", ret, errno);
            tcp_close ();
            break;

        } else if (ret == 0) {
			LOG_DBG("connection closed while waiting (%d/%d)", ret, errno);
            tcp_close ();
			break;
		}

		read_pos += ret;
		remaining -= ret;
	}

    LOG_DBG("%s recv %d bytes", proto, read_pos);
    return read_pos;
}

void Connection::tcp_close ()
{
    close (local_context.sock);
    local_context.sock = -1;
    local_context.tcp_open = false;
}

void Connection::ws_close ()
{
    close (local_context.sock_ws);
    local_context.sock_ws = -1;
    local_context.ws_open = false;
}

int Connection::tcp_send (void *user_data, size_t data_len)
{
    if (local_context.sock == -1)
    {
        LOG_ERR("tried sending, but socket is not open: %d.", local_context.sock);
        return 0;
    }

    int send_len = send (local_context.sock, user_data, data_len, 0);
    return send_len;
}

ssize_t Connection::ws_send (void *user_data, size_t data_len)
{
    ssize_t len = websocket_send_msg (
        local_context.sock_ws, 
        reinterpret_cast<const uint8_t *> (user_data),
        data_len, 
        WEBSOCKET_OPCODE_DATA_TEXT, 
        true, true, SYS_FOREVER_MS
    );

    return len;
}


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

thread__:
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


    Connection srv;
    int status = srv.tcp_open (ctx().server, ctx().port );
    while (status)
    {
        status = srv.tcp_open (ctx().server, ctx().port );
        k_sleep (K_MSEC(100));
    }

    static_string <32> buf;
    fd_set readfds;

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(srv.local_context.sock, &readfds);

        struct timeval timeout = {
            .tv_sec = 1,
            .tv_usec = 0,
        };

        int ret = select(srv.local_context.sock + 1, &readfds, NULL, NULL, &timeout);
        size_t bytes = 0;
        k_sleep (K_MSEC(50));

        if (ret > 0 && FD_ISSET(srv.local_context.sock, &readfds)) {
            buf.clear ();

            // Data is available
            LOG_INF ("select: %d, reading the data ...", ret);
            bytes = srv.tcp_recv_all (reinterpret_cast <uint8_t *>(buf.data()), buf.capacity());

            printf ("RECV bytes: %d\n", bytes);
            printf ("RECV: %s\n", (const char *)buf.c_str ());

        } else if (ret == 0) {
            // Timeout
        } else {
            // error
        }

        // try restarting the TCP connection
        if (!srv.local_context.tcp_open)
        {
            LOG_ERR ("tcp socket was closed %d. try re-open ..." , srv.local_context.sock);
            srv.tcp_open (ctx().server, ctx().port );

            while (!srv.local_context.tcp_open)
            {
                k_sleep (K_MSEC(1000));
                srv.tcp_open (ctx().server, ctx().port );
            }
        }

        k_sleep (K_MSEC(50));
    }

#if 0
    srv.ws_upgrade ();

    static_string <32> ws_payload ("{\"echo\":\"bravo\"}");
    srv.ws_send ((void *)ws_payload.c_str(), ws_payload.size());

    static_string <64> resp;
    srv.ws_recv_all (resp.data(), resp.capacity ());

    printf ("RECV: %s\n", resp.c_str ());
#endif
}

} // net
} // sys
