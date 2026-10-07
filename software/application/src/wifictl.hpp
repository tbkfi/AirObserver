#pragma once

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <stdbool.h>

namespace sys {
namespace wifi {

#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

#define NET_EVENT_WIFI_MASK \
    (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT | \
     NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT | \
     NET_REQUEST_WIFI_SCAN | NET_EVENT_WIFI_SCAN_RESULT | \
     NET_EVENT_WIFI_SCAN_DONE )

static net_if *sta_iface;
static wifi_connect_req_params sta_config;
static net_mgmt_event_callback cb;

struct context {
    char ssid[64];
    char psk[512];
    // holds last event, like 'NET_EVENT_WIFI_CONNECT_RESULT'
    long long unsigned int status; // basically uint64, but compiler is happy
    bool block; // switch to 'true' to use blocking function
};

context& ctx (void);

// static here == only visible to 'this' translation unit
static void wifi_event_handler
(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

static int wifi_init_and_connect ();

// public function
int thread (void);
void reconnect ();

} // wifi
} // sys
