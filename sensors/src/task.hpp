
/*    AirObserver
 *    task.hpp
 *    Matias Villa
 *
 */
#pragma once

#include <zephyr/kernel.h>

namespace AirObserver{

   void bme690_thread(void);
   void scd41_thread(void);
}
