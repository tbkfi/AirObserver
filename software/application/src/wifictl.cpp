
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
// #include <zephyr/net/dhcpv4_server.h>
#include <string.h>

LOG_MODULE_REGISTER (wifi, LOG_LEVEL_DBG); // logging/log.h

#include "wifictl.hpp"
#include "static_string.h"

namespace sys {
namespace wifi {

// one single instance
static context wifi_ctx ;

context& ctx (void)
{
    // this block will be executed exactly once.
    // static bool once = [] {
    //     return true;
    // }();
    return wifi_ctx;
}

static void wifi_event_handler
(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT: {
		LOG_INF("Connected to %s", sys::wifi::ctx().ssid);
        sys::wifi::ctx ().status = NET_EVENT_WIFI_CONNECT_RESULT;
		break;
	}
	case NET_EVENT_WIFI_DISCONNECT_RESULT: {
		LOG_INF("Disconnected from %s",sys::wifi::ctx().ssid);
        sys::wifi::ctx ().status = NET_EVENT_WIFI_DISCONNECT_RESULT;
		break;
	}
	case NET_EVENT_WIFI_AP_ENABLE_RESULT: {
        sys::wifi::ctx ().status = NET_EVENT_WIFI_AP_ENABLE_RESULT;
		LOG_INF("AP Mode is enabled. Waiting for station to connect");
		break;
	}
	case NET_EVENT_WIFI_AP_DISABLE_RESULT: {
        sys::wifi::ctx ().status = NET_EVENT_WIFI_AP_DISABLE_RESULT;
		LOG_INF("AP Mode is disabled.");
		break;
	}
    case NET_REQUEST_WIFI_SCAN: {
        sys::wifi::ctx ().status = NET_REQUEST_WIFI_SCAN;
		LOG_INF("WIFI scan requested.");
        break;
    }
    case NET_EVENT_WIFI_SCAN_RESULT: {
        sys::wifi::ctx ().status = NET_EVENT_WIFI_SCAN_RESULT;
        wifi_scan_result *result = (wifi_scan_result *)cb->info;

		LOG_INF("Found SSID:");

        printk("SSID: %s, Q=%ddbm, channel: %d\n",
               result->ssid,
               result->rssi,
               result->channel);

        sys::static_string <64> found_ssid (result->ssid);
        sys::static_string <64> target_ssid (sys::wifi::ctx().ssid);

        if (found_ssid == target_ssid)
            wifi_init_and_connect ();

        // if scan is issued, and an appropriate network is found
        // the device tries connecting to it.
        // if (!strcmp (reinterpret_cast <const char *>result->ssid, sys::wifi::ctx ().ssid))

        break;
    }
    case NET_EVENT_WIFI_SCAN_DONE: {
        sys::wifi::ctx ().status = NET_EVENT_WIFI_SCAN_DONE;
		LOG_INF("WIFI scan complete.");
        break;
    }
#if 0
	case NET_EVENT_WIFI_AP_STA_CONNECTED: {
		struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

		LOG_INF("station: " MACSTR " joined ", sta_info->mac[0], sta_info->mac[1],
			sta_info->mac[2], sta_info->mac[3], sta_info->mac[4], sta_info->mac[5]);
		break;
	}
	case NET_EVENT_WIFI_AP_STA_DISCONNECTED: {
		struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

		LOG_INF("station: " MACSTR " leave ", sta_info->mac[0], sta_info->mac[1],
			sta_info->mac[2], sta_info->mac[3], sta_info->mac[4], sta_info->mac[5]);
		break;
	}
#endif
	default:
		break;
	}
}

static int wifi_init_and_connect ()
{
	if (sta_iface == NULL) {
		LOG_INF("STA: interface no initialized");
		return -EIO;
	}
    auto& c_wifi = sys::wifi::ctx();

    // am I tripping ?
    // memcpy ((void *)sta_config->ssid, c_wifi.ssid, strlen (c_wifi.ssid) + 1 );
	// memcpy ((void *)sta_config->psk, c_wifi.psk, strlen (c_wifi.psk) + 1 );
    sta_config.ssid = reinterpret_cast <uint8_t *> (c_wifi.ssid);
    sta_config.psk = reinterpret_cast <uint8_t *> (c_wifi.psk);

	sta_config.ssid_length = strlen (c_wifi.ssid);
	sta_config.psk_length = strlen (c_wifi.psk);

	// sta_config->ssid = (const uint8_t *)CONFIG_WIFI_SAMPLE_SSID;
	// sta_config->ssid_length = sizeof(CONFIG_WIFI_SAMPLE_SSID) - 1;
	// sta_config->psk = (const uint8_t *)CONFIG_WIFI_SAMPLE_PSK;
	// sta_config->psk_length = sizeof(CONFIG_WIFI_SAMPLE_PSK) - 1;

	sta_config.security = WIFI_SECURITY_TYPE_PSK;
	sta_config.channel = WIFI_CHANNEL_ANY;
	sta_config.band = WIFI_FREQ_BAND_2_4_GHZ;

	LOG_INF("Connecting to SSID: %s\n", sta_config.ssid);

	int ret = net_mgmt (NET_REQUEST_WIFI_CONNECT, sta_iface, &sta_config,
			   sizeof(struct wifi_connect_req_params));

	if (ret) {
		LOG_ERR("Unable to Connect to (%s)", sta_config.ssid);
	}

	return ret;
}

int thread ()
{
    LOG_INF ("start wireless communication");

    while (sys::wifi::ctx().ssid == nullptr)
    {
        k_sleep (K_MSEC (1000));  // sleep for a second
        LOG_INF ("SSID is NULL, waiting for data ...");
    }

    // should be updated every once per

    LOG_INF ("initializing the network stack and trying to connect ...");

	net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
	net_mgmt_add_event_callback(&cb);

	/* Get STA interface in AP-STA mode. */
	sta_iface = net_if_get_wifi_sta ();

	int ret = net_mgmt
    (NET_REQUEST_WIFI_SCAN, sta_iface, &sta_config, sizeof(wifi_connect_req_params));

    // while (1)
    // {
    //     k_sleep (K_MSEC (1000));  // sleep for a second
    // }
    LOG_INF ("thread successfully started.");
	return ret;
}
void reconnect ()
{
	LOG_INF("Issue reconnect to: %s\n", sta_config.ssid);

	int ret = net_mgmt (NET_REQUEST_WIFI_CONNECT, sta_iface, &sta_config,
			   sizeof(struct wifi_connect_req_params));

	if (ret) {
		LOG_ERR("Unable to Connect to (%s)", sta_config.ssid);
	}
    
}

} // wifi 
} // sys
