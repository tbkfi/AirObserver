
/*    AirObserver
 *    util.cpp
 *    Matias Villa
 */

#include <zephyr/kernel.h>
#include "util.hpp"
#include "bme690.hpp"
#include "scd41.hpp"

namespace UTIL {
   K_MUTEX_DEFINE(air_mutex);
   // BME690 
   K_WORK_DELAYABLE_DEFINE(gas_work, BME690::fetch_work_handler);
   K_WORK_DELAYABLE_DEFINE(meas_steps, BME690::measurement_work_handler);

   // SCD41
   K_WORK_DELAYABLE_DEFINE(meas_scd41, SCD41::fetch_scd41_readings);
   
   Context_util CTX_UTIL;
   Context_util& util_ctx(void) {return CTX_UTIL;}
}
