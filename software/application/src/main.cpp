
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

int main(void)
{
    auto& ctx_wifi = sys::ctx ().wifi_data ();

    char new_ssid[] = "htspot";
    char new_passwd[] = "netconn1337";

    memcpy (ctx_wifi.ssid, new_ssid, sizeof (new_ssid));
    memcpy (ctx_wifi.psk, new_passwd, sizeof (new_passwd));

    k_thread_start (wifictl_id);
    
    // wait for wifi connectivity.
    while (ctx_wifi.status != NET_EVENT_WIFI_CONNECT_RESULT)
        k_sleep (K_MSEC (1000));  // sleep for a second

    Connection srv;
    srv.tcp_open ("10.213.20.57", 1337);
    
    static_string <12> payload ("echo\n");
    srv.raw_send ((void *)payload.c_str (), payload.size ());

	return 0;
}

