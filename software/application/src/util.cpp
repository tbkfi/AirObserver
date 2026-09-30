
/*    AirObserver
 *    util.cpp
 *    Matias Villa
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include "util.hpp"

namespace sys {
namespace util {

    // static struct k_mutex air_mutex_global;
    static context ctx_util;
    static bool __init_done = false;

    context& ctx(void)
    {
        if (!__init_done)
        {   // ghetto ! kolhoz !
            k_mutex_init(&ctx_util.air_mutex);
            __init_done = true;
        }
        return ctx_util;
    }
} // util


} // sys
