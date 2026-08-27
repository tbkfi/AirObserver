/*
 * AirObserver
 * bme690.cpp
 * Created by Matias Villa
 *
 */

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include "bme690.hpp"
#include "util.hpp"

#define I2C0_NODE DT_NODELABEL(bme690)

namespace BME690
{
   const struct i2c_dt_spec bme_dev = I2C_DT_SPEC_GET(I2C0_NODE);
   struct gas_info          gas_calib;
   struct gas_parsed        curated_gas;
   measurement_steps        current_meas_step;
   struct fetch_flag        gas_was_fetched;

   static size_t step_idx         = 0;
   static int    retry_count      = 0;
   static int    meas_retry_count = 0;

   static struct gas_fetched steps[] = {
      { PAR_G1,         &gas_calib.par_g1,    sizeof(gas_calib.par_g1),    &gas_was_fetched.par_g1    },
      { PAR_G2,         &gas_calib.par_g2,    sizeof(gas_calib.par_g2),    &gas_was_fetched.par_g2    },
      { PAR_G3,         &gas_calib.par_g3,    sizeof(gas_calib.par_g3),    &gas_was_fetched.par_g3    },
      { RES_HEAT_RANGE, &gas_calib.raw_range, sizeof(gas_calib.raw_range), &gas_was_fetched.res_range },
      { RES_HEAT_VALUE, &gas_calib.raw_val,   sizeof(gas_calib.raw_val),   &gas_was_fetched.res_heat  },
   };

   void fetch_work_handler(struct k_work *work) {
      struct gas_fetched *s = &steps[step_idx];

      int rc = i2c_write_read_dt(&bme_dev, &s->reg, 1, s->dest, s->len);

      if (rc == 0) {
         *s->fetched_flag = true;
         step_idx++;
         retry_count = 0;
      } else {
         printk("I2C error on step %u (reg 0x%02x): [%d]\n", step_idx, s->reg, rc);
         retry_count++;
         if (retry_count >= I2C_RETRY) {
            *s->fetched_flag = false;   // give up on this one, move on so we don't hang forever
            step_idx++;
            retry_count = 0;
         }
         // else: step_idx unchanged -> same step gets retried on next schedule
      }

      if (step_idx < ARRAY_SIZE(steps)) {
         k_work_schedule(&UTIL::gas_work, K_MSEC(I2C_WAIT_MS));
      } else {
         gas_calib.res_range = (gas_calib.raw_range >> 4) & 0x03;   // bits <5:4>
         gas_calib.res_heat  = gas_calib.raw_val;
         step_idx = 0;   // reset for next fetch cycle
         k_sem_give(&UTIL::sem_gas);
      }
   }

   void configure_oversampling(void) {
      uint8_t ctrl_meas_val[2] = {CTRL_MEAS, CTRL_MEAS_OSRS};
      i2c_write_dt(&bme_dev, ctrl_meas_val, sizeof(ctrl_meas_val));
   }

   void soft_reset(void) {
      uint8_t buf[2] = {RESET_REG, RESET_CMD};
      i2c_write_dt(&bme_dev, buf, sizeof(buf));
   }

   void calc_res_heat(void) {
      // Formula provided by Bosch in the datasheet
      // Calculate the values necessary for sensor calibration
      // Heat resistance calculation

      double var1       = ((double)gas_calib.par_g1 / 16.0) + 49.0;
      double var2       = (((double)gas_calib.par_g2 / 32768.0) * 0.0005) + 0.00235;
      double var3       = (double)gas_calib.par_g3 / 1024.0;
      double var4       = var1 * (1.0 + (var2 * (double)TARGET_TEMP));
      double var5       = var4 + (var3 * (double)AMB_TEMP);
      double res_heat_x = (uint8_t)(3.4 * ((var5 * (4.0 / (4.0 + (double)gas_calib.res_range))) *
                           (1.0 / (1.0 + (double)gas_calib.res_heat * 0.002))) - 25);
      // sensor is calibrated
      gas_calib.is_calib = res_heat_x;
   }

   void measurement_work_handler(struct k_work *measurements) {
      uint8_t gas_wait_0[2] = {GAS_WAIT_0, 0x59};
      uint8_t res_heat_0[2] = {RES_HEAT_0, (uint8_t)gas_calib.is_calib};
      uint8_t ctrl_gas_1    = CTRL_GAS_1;
      uint8_t ctrl_meas     = CTRL_MEAS;
      uint8_t run_gas;
      uint8_t set_mode;
      int rc = 0;

      switch (current_meas_step) {

      case GAS_WAIT_X:
         rc = i2c_write_dt(&bme_dev, gas_wait_0, sizeof(gas_wait_0));
         if (rc != 0) {
            printk("Error writing gas_wait_0: [%d]\n", rc);
            if (++meas_retry_count < I2C_RETRY) {
               k_work_schedule(&UTIL::meas_steps, K_MSEC(I2C_WAIT_MS));
               return;   // retry same step
            }
            printk("Giving up on gas_wait_0 after %d retries\n", I2C_RETRY);
         }
         meas_retry_count  = 0;
         current_meas_step = RES_HEAT_X;
         k_work_schedule(&UTIL::meas_steps, K_MSEC(I2C_WAIT_MS));
         break;

      case RES_HEAT_X:
         rc = i2c_write_dt(&bme_dev, res_heat_0, sizeof(res_heat_0));
         if (rc != 0) {
            printk("Error writing res_heat_0: [%d]\n", rc);
            if (++meas_retry_count < I2C_RETRY) {
               k_work_schedule(&UTIL::meas_steps, K_MSEC(I2C_WAIT_MS));
               return;
            }
            printk("Giving up on res_heat_0 after %d retries\n", I2C_RETRY);
         }
         meas_retry_count  = 0;
         current_meas_step = RUN_GAS;
         k_work_schedule(&UTIL::meas_steps, K_MSEC(I2C_WAIT_MS));
         break;

      case RUN_GAS:
         rc = i2c_write_read_dt(&bme_dev, &ctrl_gas_1, 1, &run_gas, 1);
         if (rc != 0) {
            printk("Error reading ctrl_gas_1: [%d]\n", rc);
            if (++meas_retry_count < I2C_RETRY) {
               k_work_schedule(&UTIL::meas_steps, K_MSEC(I2C_WAIT_MS));
               return;
            }
            printk("Giving up on ctrl_gas_1 after %d retries\n", I2C_RETRY);
            meas_retry_count  = 0;
            current_meas_step = NB_CONV;   // don't hang forever, but run_gas bit is unset
            k_work_schedule(&UTIL::meas_steps, K_MSEC(I2C_WAIT_MS));
            break;
         }
         run_gas &= ~0x0F;     // clear nb_conv index to 0
         run_gas |= (1 << 5);  // set run_gas bit
         {
            uint8_t buf[2] = {CTRL_GAS_1, run_gas};
            i2c_write_dt(&bme_dev, buf, sizeof(buf));
         }
         meas_retry_count  = 0;
         current_meas_step = NB_CONV;
         k_work_schedule(&UTIL::meas_steps, K_MSEC(I2C_WAIT_MS));
         break;

      case NB_CONV:
         rc = i2c_write_read_dt(&bme_dev, &ctrl_meas, 1, &set_mode, 1);
         if (rc != 0) {
            printk("Error reading ctrl_meas register: [%d]\n", rc);
            if (++meas_retry_count < I2C_RETRY) {
               k_work_schedule(&UTIL::meas_steps, K_MSEC(I2C_WAIT_MS));
               return;
            }
            printk("Giving up on ctrl_meas after %d retries\n", I2C_RETRY);
            meas_retry_count = 0;
            k_sem_give(&UTIL::sem_meas);   // don't leave bme690_thread blocked forever
            break;
         }
         set_mode &= ~0x03;   // Clear bits 1:0
         set_mode |= 0x01;    // Set Forced Mode (0b01)
         {
            uint8_t buf2[2] = {CTRL_MEAS, set_mode};
            i2c_write_dt(&bme_dev, buf2, sizeof(buf2));
         }
         meas_retry_count = 0;
         k_sem_give(&UTIL::sem_meas);
         break;
      }
   }

   bool new_gas_readout(void) {
      // IF HEAT_STAB_R IS EQUAL TO 1, SENSOR IS READY TO MEASURE GAS IN ENVIROEMENT
      uint8_t gas_r_lsb = 0x2D;
      uint8_t heat_stab_r;

      int rc = i2c_write_read_dt(&bme_dev, &gas_r_lsb, 1, &heat_stab_r, 1);

      if (rc != 0) {
         printk("Error writing to register\n");
         return false;
      } else {
         if ((heat_stab_r & 16) != 16) {
            // CHECK IF VALUE OF BIT NUMBER 4 IS 1, IF 0 THERE IS NO VALID DATA.
            printk("There is no valid data. Value of bit 4 is: 0\n");
            return false;
         }
         printk("Sensor ready to perform readings!\n");
         curated_gas.heat_stab_reg = heat_stab_r;
         return true;
      }
   }

   void parse_gas_readings(void) {
      // CONVERT GAS RESISTANCE MEASUREMENT TO OHMS.
      // ELI5 EXPLANATION: THE GREATER THE RESISTANCE, CLEANER THE AIR.
      // YET TO IMPLEMENT A MORE INTUITIVE READING OF GAS IN ENVIROEMENT.
      uint8_t  msb_reg = 0x2C;
      uint8_t  lsb_reg = 0x2D;
      uint8_t  gas_r_msb;
      uint8_t  gas_r_lsb;
      uint16_t gas_adc;
      uint8_t  gas_range;

      int err = i2c_write_read_dt(&bme_dev, &msb_reg, 1, &gas_r_msb, 1);
      if (err != 0) printk("Error reading gas_r_msb\n");

      err = i2c_write_read_dt(&bme_dev, &lsb_reg, 1, &gas_r_lsb, 1);
      if (err != 0) printk("Error reading gas_r_lsb\n");

      gas_adc   = ((uint16_t)gas_r_msb << 2) | (gas_r_lsb >> 6);   // "Lives" on bit <7:6>
      gas_range = gas_r_lsb & 0x0F;                                // "Lives" on bits <3:0>

      // DEBUG/CURIOSITY PRINT
      printk("gas_adc: [%d], gas_range: [%d]\n", gas_adc, gas_range);

      curated_gas.adc_gas   = gas_adc;
      curated_gas.range_gas = gas_range;

      // FORMULA PROVIDED BY BOSCH DATASHEET ->
      // CONVERT ANALOG READS INTO OHMS
      uint32_t var1 = UINT32_C(262144) >> gas_range;
      int32_t  var2 = (int32_t)gas_adc - INT32_C(512);
      var2 *= INT32_C(3);
      var2 = INT32_C(4096) + var2;
      float gas_res = 1000000.0f * (float)var1 / (float)var2;
      gas_calib.gas_ohms = gas_res;
   }

   void run_bme690_readings(void) {
      // EXPERIMENTAL GAS SENSOR SEQUENCE (FIELD 0)
      printk("Fetching values to perform calibration...\n");
      current_meas_step = GAS_WAIT_X;

      k_work_schedule(&UTIL::gas_work, K_NO_WAIT);
      k_work_schedule(&UTIL::meas_steps, K_NO_WAIT);

      k_sem_take(&UTIL::sem_gas, K_FOREVER);
      k_sem_take(&UTIL::sem_meas, K_FOREVER);

      printk("Calculating res_heat_x\n");
      calc_res_heat();
      printk("res_heat_x: [%d]\n", gas_calib.is_calib);

      printk("Enabling gas measurment...\n");
      configure_oversampling();
   }
}
