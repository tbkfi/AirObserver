/*    AirObserver
 *    task.cpp
 *    Matias Villa
 *
 */

#include <zephyr/kernel.h>
#include "scd41.hpp"
#include "bme690.hpp"
#include "task.hpp"

void bme690_thread(void){
   
   
   while (1){
      
      BME690::run_bme690_readings();
      printk("Performing gas resistance measurement...\n");

      bool is_ready = BME690::new_gas_readout();
      
      if (is_ready) {
         BME690::parse_gas_readings();
         printk("Gas resistance level is: [%.3f] ohms\n", (double)BME690::gas_calib.gas_ohms);
      } 
      else printk("Measurement not ready this cycle, skipping.\n");
   }

}

void scd41_thread(void){

   while (1){
  
      if (sensor_sample_fetch(SCD41::dev) < 0) {
         printk("Failed to fetch sample\n");
         continue;
      }

      SCD41::run_scd41_readings();   
   }
}


