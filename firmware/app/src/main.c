#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>

#include <my_cli/w5500_cmds.h>
#include <net_w5500/net_w5500.h>
#include <net_w5500_led/net_w5500_led.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);


// LED fault codes
#define FAULT_NET_W5500_INIT   2
#define FAULT_NET_W5500_THREAD 3
#define FAULT_NO_IP_LEASE      4


int main(void) {
	int rc;

	// Application and Module ctx
	// TODO: app by Pavel
	static struct net_w5500_ctx w5500_ctx;

	// W5500 Setup
	rc = net_w5500_led_init();
	if (rc < 0) {
		// Warn is enough
		LOG_WRN("net_w5500_led_init failed: %d (status LEDs unavailable)", ret);
	}

	w5500_ctx.iface = net_if_get_default();
	rc = net_w5500_init(&w5500_ctx);
	if (rc < 0) {
		LOG_ERR("net_w5500_init failed: %d", ret);
		net_w5500_led_signal_fault(FAULT_NET_W5500_INIT);
	}

	rc = net_w5500_thread_create(&w5500_ctx);
	if (rc < 0) {
		LOG_ERR("net_w5500_thread_create failed: %d", ret);
		net_w5500_led_signal_fault(FAULT_NET_W5500_THREAD);
	}

	w5500_cmds_set_ctx(&w5500_ctx);

	rc = net_w5500_start_dhcp(&w5500_ctx);
	if (rc < 0) {
		LOG_ERR("net_w5500_start_dhcp failed: %d", ret);
	}

	rc = net_w5500_wait_for_ip(&w5500_ctx, K_SECONDS(30));
	if (rc < 0) {
		LOG_WRN("no IPv4 address assigned within timeout: %d", ret);
		net_w5500_led_signal_fault(FAULT_NO_IP_LEASE);
	}

	LOG_INF("IPv4 address assigned");
	net_w5500_led_set_power(true);
	net_w5500_led_pulse_activity();

	LOG_INF("Boot Sequence Complete");

	return 0;
}
