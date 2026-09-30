#pragma once

#include "bme690.hpp"
#include "scd41.hpp"
#include "util.hpp"
#include "wifictl.hpp"

// made by Don Pablo

namespace sys {

// I want to have a kind of singleton, which is simple.
// So I could mark it static and go on with my life.
// manager

// system_state exists for managing system context.
struct system_state {
    sensor::BME690::context& bme_data ()
    {
        return sensor::BME690::ctx ();
    }

    sensor::SCD41::context& scd_data ()
    {
        return sensor::SCD41::ctx ();
    }

    util::context& util_data ()
    {
        return util::ctx ();
    }

    wifi::context& wifi_data ()
    {
        return wifi::ctx ();
    }
};

system_state& ctx ();
} // namespace sys
