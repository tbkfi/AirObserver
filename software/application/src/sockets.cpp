
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
#include "context.hpp"

LOG_MODULE_REGISTER(tcp_sockets, LOG_LEVEL_DBG);

namespace sys {
namespace transport {

static int connect_cb (int sock, struct http_request *req, void *user_data)
{
	LOG_INF("websocket %d for %s connected.", sock, (char *)user_data);
	return 0;
}

// open TCP socket, but do not send anything
int connection::tcp_open (const char *server, int port)
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

        close (local_context.sock);
        local_context.sock = -1;
        ret = -errno;
        return ret;
    }

    LOG_INF ("socket open [%d] for %s (%d)", local_context.sock, server, -errno);
    local_context.tcp_open = true;
    return ret;
}

// TODO: WS upgrade
int connection::ws_open (const char *server, int port)
{
    if (!local_context.tcp_open)
    {
        int ret = tcp_open (server, port);

        if (ret)
        {
            LOG_ERR ("failed to open websocket (%d): %s", 
                -errno, strerror(errno));

            return -errno;
        }
    }

    static uint8_t tmp_buf [512] = {};
    const char *extra_headers[] = {"Origin: http://foobar\r\n", NULL };
    websocket_request req {};

    req.host = local_context.server.c_str ();
    req.url = "/";
    req.optional_headers = extra_headers;
    req.cb = connect_cb;
    req.tmp_buf = tmp_buf;
    req.tmp_buf_len = sizeof tmp_buf;

    local_context.sock_ws = websocket_connect(local_context.sock, &req, 100, (void *)"IPv4");

    if (local_context.sock_ws < 0) {
        LOG_ERR("Cannot connect to %s:%d", local_context.server.c_str(), local_context.port);
        tcp_close ();
        return local_context.sock;
    }

    local_context.ws_open = true;
    LOG_INF ("websocket open [%d] for %s (%d)", local_context.sock_ws, server, local_context.sock);
    return local_context.sock_ws;
}

void connection::ws_recv
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


int connection::tcp_recv
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

int connection::tcp_recv_all (uint8_t *buf, size_t buf_len) // DONE
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

void connection::tcp_close ()
{
    close (local_context.sock);
    local_context.sock = -1;
    local_context.tcp_open = false;

    // websocket cannot exist without TCP socket.
    if (local_context.ws_open)
        ws_close ();
}

void connection::ws_close ()
{
    close (local_context.sock_ws);
    local_context.sock_ws = -1;
    local_context.ws_open = false;
}

int connection::tcp_send (void *user_data, size_t data_len)
{
    if (local_context.sock == -1)
    {
        LOG_ERR("tried sending, but socket is not open: %d.", local_context.sock);
        return 0;
    }

    int send_len = send (local_context.sock, user_data, data_len, 0);
    return send_len;
}

ssize_t connection::ws_send (void *user_data, size_t data_len)
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

int connection::ws_recv_all (uint8_t *buf, size_t buf_len)
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
			if (ret == -EAGAIN || ret == -EWOULDBLOCK) {
                // no data to read, do not block
                break;
			}

            if (ret == -ENOTCONN)
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
		total_read += ret;
	}

    LOG_DBG("%s recv %d bytes", proto, total_read);
    return total_read;
}

} // transport
} // sys
