#include "wifictl.h"
#include <zephyr/kernel.h>

//K_THREAD_DEFINE (name, stack_size, entry, p1, p2, p3, prio, options, delay )
K_THREAD_DEFINE (wifictl_id, 4096, wifictl, 1, NULL, NULL, 7, 0, K_TICKS_FOREVER);

int main(void)
{
    int s = wifictl (1);

    k_thread_start (wifictl_id);
	return 0;
}

