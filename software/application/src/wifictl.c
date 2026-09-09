
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
// #include <zephyr/net/dhcpv4_server.h>

LOG_MODULE_REGISTER (wifi, LOG_LEVEL_DBG); // logging/log.h


#include "wifictl.h"

static void wifi_event_handler
(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT: {
		LOG_INF("Connected to %s", CONFIG_WIFI_SAMPLE_SSID);
		break;
	}
	case NET_EVENT_WIFI_DISCONNECT_RESULT: {
		LOG_INF("Disconnected from %s", CONFIG_WIFI_SAMPLE_SSID);
		break;
	}
	case NET_EVENT_WIFI_AP_ENABLE_RESULT: {
		LOG_INF("AP Mode is enabled. Waiting for station to connect");
		break;
	}
	case NET_EVENT_WIFI_AP_DISABLE_RESULT: {
		LOG_INF("AP Mode is disabled.");
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

static int wifi_init_and_connect
(struct net_if* sta_iface, struct wifi_connect_req_params* sta_config)
{
	if (sta_iface == NULL) {
		LOG_INF("STA: interface no initialized");
		return -EIO;
	}

	sta_config->ssid = (const uint8_t *)CONFIG_WIFI_SAMPLE_SSID;
	sta_config->ssid_length = sizeof(CONFIG_WIFI_SAMPLE_SSID) - 1;
	sta_config->psk = (const uint8_t *)CONFIG_WIFI_SAMPLE_PSK;
	sta_config->psk_length = sizeof(CONFIG_WIFI_SAMPLE_PSK) - 1;
	sta_config->security = WIFI_SECURITY_TYPE_PSK;
	sta_config->channel = WIFI_CHANNEL_ANY;
	sta_config->band = WIFI_FREQ_BAND_2_4_GHZ;

	LOG_INF("Connecting to SSID: %s\n", sta_config->ssid);

	int ret = net_mgmt (NET_REQUEST_WIFI_CONNECT, sta_iface, sta_config,
			   sizeof(struct wifi_connect_req_params));

	if (ret) {
		LOG_ERR("Unable to Connect to (%s)", CONFIG_WIFI_SAMPLE_SSID);
	}

	return ret;
}

static int wifi_init_and_connect_blocking
(struct net_if* sta_iface, struct wifi_connect_req_params* sta_config)
{
	if (sta_iface == NULL) {
		LOG_INF("STA: interface no initialized");
		return -EIO;
	}

	sta_config->ssid = (const uint8_t *)CONFIG_WIFI_SAMPLE_SSID;
	sta_config->ssid_length = sizeof(CONFIG_WIFI_SAMPLE_SSID) - 1;
	sta_config->psk = (const uint8_t *)CONFIG_WIFI_SAMPLE_PSK;
	sta_config->psk_length = sizeof(CONFIG_WIFI_SAMPLE_PSK) - 1;
	sta_config->security = WIFI_SECURITY_TYPE_PSK;
	sta_config->channel = WIFI_CHANNEL_ANY;
	sta_config->band = WIFI_FREQ_BAND_2_4_GHZ;

	LOG_INF("Connecting to SSID: %s\n", sta_config->ssid);

	int ret = net_mgmt (NET_REQUEST_WIFI_CONNECT, sta_iface, sta_config,
			   sizeof(struct wifi_connect_req_params));

    unsigned ctr = 0;
    while (ret)
    {
        ctr ++;
		if (!(ctr % 10)) // practically, print every second
            LOG_ERR("Unable to Connect to (%s).", CONFIG_WIFI_SAMPLE_SSID);


        // block until the connection is successful
        ret = net_mgmt 
        (NET_REQUEST_WIFI_CONNECT, sta_iface, sta_config, sizeof(struct wifi_connect_req_params));

        k_sleep(K_MSEC (100));

        if (ctr == -1) ctr = 0; // if UINT_MAX, wrap around
    }

	return ret;
}

int wifictl (bool block)
{
    LOG_INF ("start wireless communication");

	net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
	net_mgmt_add_event_callback(&cb);

	/* Get STA interface in AP-STA mode. */
	sta_iface = net_if_get_wifi_sta ();

    int status = 0;
    if (block) {
        status = wifi_init_and_connect_blocking (sta_iface, &sta_config);
    } else {
        status = wifi_init_and_connect (sta_iface, &sta_config);
    }

	return status;
}

