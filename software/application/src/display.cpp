
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/devicetree.h>

#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER (display, LOG_LEVEL_INF);

namespace sys {
namespace display {

// NOTE: now, worse is better, make something simple,
// without going crazy, like always.
context& ctx (void)
{
    return display_ctx;
}

void thread (void)
{
    // Fetch the device binding using the devicetree alias
    const struct device *display_dev = DEVICE_DT_GET(DT_ALIAS(display_controller));

    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device %s is not ready!", display_dev->name);
        return -EIO;
    }

    LOG_INF("SSD1680 Display successfully initialized via SPI.");
    
    // Optional: Blank or clear screen capabilities can go here
    display_blanking_off(display_dev);

    return 0;
}
} // display
} // sys
