#pragma once

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <stdbool.h>

#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

#define NET_EVENT_WIFI_MASK \
    (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT | \
     NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT | \
     NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)


// even though static, it is still internal linkage
static struct net_if *sta_iface;
static struct wifi_connect_req_params sta_config;
static struct net_mgmt_event_callback cb;

// Check necessary definitions
BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_AP_SSID) > 1,
         "CONFIG_WIFI_SAMPLE_AP_SSID is empty. Please set it in conf file.");
BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_SSID) > 1,
         "CONFIG_WIFI_SAMPLE_SSID is empty. Please set it in conf file.");

// static here == only visible to 'this' translation unit
static void wifi_event_handler
(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

static int wifi_init_and_connect 
(struct net_if* iface_sta, struct wifi_connect_req_params* config_sta);

static int wifi_init_and_connect_blocking
(struct net_if* iface_sta, struct wifi_connect_req_params* config_sta);

// public function
int wifictl (bool block);
