#include "context.hpp"
#include "bme690.hpp"
#include "util.hpp"

namespace sys {
static system_state sys_ctx;

// this method exists only for controlling system_state.
system_state& ctx ()
{
    return sys_ctx;
}

} // sys

// auto& c = sys::ctx ();
// c.wifi_data.psk = echo;
// c.bme_data.register = bravo;
