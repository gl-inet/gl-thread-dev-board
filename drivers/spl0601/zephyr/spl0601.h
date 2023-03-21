/*****************************************************************************
 * @file  spl0601.h
 * @brief The header file of spl0601.c
 *******************************************************************************
 Copyright 2022 GL-iNet. https://www.gl-inet.com/

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ******************************************************************************/

#ifndef _SPL0601_H_
#define _SPL0601_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#define SPL0601_HW_ADR 0x77
#define SPL0601_CONTINUOUS_PRESSURE 1
#define SPL0601_CONTINUOUS_TEMPERATURE 2
#define SPL0601_CONTINUOUS_P_AND_T 3
#define SPL0601_PRESSURE_SENSOR 0
#define SPL0601_TEMPERATURE_SENSOR 1

#define SPL0601_REG_ID 0x0D
#define SPL0601_DEFAULT_ID 0x10

#define SPL0601_SAMPLING_RATE (32) // 32 Hz

#define SPL0601_0_OVERSAMPLE_MEASUREMENT_T (4) // 3.6 ms
#define SPL0601_2_OVERSAMPLE_MEASUREMENT_T (6) // 5.2 ms
#define SPL0601_4_OVERSAMPLE_MEASUREMENT_T (9) // 8.4 ms
#define SPL0601_8_OVERSAMPLE_MEASUREMENT_T (15) // 14.8 ms
#define SPL0601_16_OVERSAMPLE_MEASUREMENT_T (28) // 27.6 ms
#define SPL0601_32_OVERSAMPLE_MEASUREMENT_T (54) // 53.2 ms
#define SPL0601_64_OVERSAMPLE_MEASUREMENT_T (105) // 104.4 ms
#define SPL0601_128_OVERSAMPLE_MEASUREMENT_T (207) // 206.8 ms

typedef enum spl0601_ovsample {
	OV_SINGLE = 0,
	OV_2 = 2,
	OV_4 = 4,
	OV_8 = 8,
	OV_16 = 16,
	OV_32 = 32,
	OV_64 = 64,
	OV_128 = 128,
} spl0601_ovsample_e;

struct spl0601_config {
	struct i2c_dt_spec i2c;
};

struct spl0601_data {
	struct k_sem sem;

	/* Compensation parameters. */
	int16_t c0;
	int16_t c1;
	uint32_t c00;
	int32_t c10;
	int16_t c01;
	int16_t c11;
	int16_t c20;
	int16_t c21;
	int16_t c30;
	uint32_t i32kP;
	uint32_t i32kT;

	/* Time for waitting sensor values. Unit: ms*/
	uint16_t wait_time;

	/* Calculated sensor values. */
	float calc_pressure;
	float calc_temperature;

	/* Raw sensor values. */
	int32_t raw_pressure;
	int32_t raw_temperature;

	uint8_t chip_id;
};

#ifdef __cplusplus
}
#endif

#endif /* _SPL0601_H_ */