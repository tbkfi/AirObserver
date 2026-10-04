
#include "wifictl.hpp"
#include "context.hpp"

#include <zephyr/kernel.h>

#include <stdio.h>
#include <string.h>

#include "sockets.hpp"
#include "static_string.h"

//K_THREAD_DEFINE (name, stack_size, entry, p1, p2, p3, prio, options, delay )
//K_THREAD_DEFINE (bme690_id, 4096, sys::sensor::BME690::thread, NULL, NULL, NULL, 7, 0, K_TICKS_FOREVER);
K_THREAD_DEFINE (wifictl_id, 4096, sys::wifi::thread, 1, NULL, NULL, 7, 0, K_TICKS_FOREVER);
K_THREAD_DEFINE (net_id, 4096, sys::net::thread, 1, NULL, NULL, 7, 0, K_TICKS_FOREVER);

int main(void)
{
    auto& ctx_wifi = sys::ctx ().wifi_data ();

    char new_ssid[] = "htspot";
    char new_passwd[] = "netconn1337";

    memcpy (ctx_wifi.ssid, new_ssid, sizeof (new_ssid));
    memcpy (ctx_wifi.psk, new_passwd, sizeof (new_passwd));

    char server[] = "10.200.213.57";
    int port = 1337;

    memcpy (sys::ctx().net_data ().server, server, sizeof (server));
    sys::ctx().net_data ().port = port;

    k_thread_start (wifictl_id);
    k_thread_start (net_id);
    
	return 0;
}

