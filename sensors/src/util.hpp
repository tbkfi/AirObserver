/*    AirObserver
 *    util.hpp
 *    Matias Villa
 */
#pragma once

#ifndef UTIL_HPP
#define UTIL_HPP

#include <zephyr/kernel.h>

#define  I2C_WAIT_MS    5

namespace UTIL {
   extern struct k_mutex air_mutex;
}

#endif
