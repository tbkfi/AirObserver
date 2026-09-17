
#include "wifictl.hpp"
#include <zephyr/kernel.h>

#include "context.hpp"
#include <stdio.h>

//K_THREAD_DEFINE (name, stack_size, entry, p1, p2, p3, prio, options, delay )
K_THREAD_DEFINE (wifictl_id, 4096, wifictl, 1, NULL, NULL, 7, 0, K_TICKS_FOREVER);
K_THREAD_DEFINE (bme690_id, 4096, bme690_thread, NULL, NULL, NULL, 7, 0, K_TICKS_FOREVER);

int main(void)
{
    int s = wifictl (1);

    auto& ctx = sys::ctx::instance ();
    ctx.a = 10;
    sys::ctx::instance ().a = 20;

    printf ("a: %d\n", sys::ctx::instance().a);
    printf ("a:ref: %d\n", ctx.a);

    k_thread_start (wifictl_id);
	return 0;
}

