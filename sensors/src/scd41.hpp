/*
 *  AirObserver 
 *  scd41.hpp  
 *  Created by Matias Villa
 *
 *  INITIAL TESTING OF THE SCD41: CO2, TEMPERATURE AND HUMIDITY SENSOR.
*/

#pragma once

#ifndef SCD41_HPP
#define SCD41_HPP

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/drivers/sensor/scd4x.h>


namespace SCD41
{

   bool force_scd41_recalib(void);
   void fetch_scd41_readings(struct k_work *read_scd41);
   void run_scd41_readings(void);;

   struct scd41_readings {
      struct sensor_value co2;
      struct sensor_value temp;
      struct sensor_value humidity;
   };

   struct recalib_values {
      uint16_t correction;
      uint16_t target_ppm;
   };
   
   typedef enum {
      CO2,
      TEMP,
      HUMIDITY,
      PRINT_DATA
   } step_readings;

   extern const struct device *dev;
   extern struct scd41_readings enviroment_data;

}   

#endif // SCD41_H
