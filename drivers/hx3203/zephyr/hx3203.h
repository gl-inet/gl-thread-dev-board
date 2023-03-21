/*****************************************************************************
 * @file  hx3203.h
 * @brief The header file of hx3203.c
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

#ifndef _HX3203_H_
#define _HX3203_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>


#define HX3203_DEFAULT_DEVICE_ID 0x21
#define HX3203_REG_DEVICE_ID 0x00
#define HX3203_REG_ENABLE_ALS_PS 0x01
#define HX3203_REG_INT_CTL 0x02
#define HX3203_REG_ALS_INT_LOW_THD_11_4 0x05
#define HX3203_REG_ALS_INT_HIGH_THD_7_4_AND_LOW_THD_15_12 0x06
#define HX3203_REG_ALS_INT_HIGH_THD_15_8 0x07
#define HX3203_REG_CH1_DATA_10_3 0x08
#define HX3203_REG_CH0_DATA_15_8 0x09
#define HX3203_REG_CH0_DATA_7_4 0x0A
#define HX3203_REG_ALS_GAIN_CTL 0x0B
#define HX3203_REG_CH1_DATA_17_11 0x0D
#define HX3203_REG_CH1_DATA_2_0 0x0E
#define HX3203_REG_CH0_DATA_17_16_AND_3_0 0x0F
#define HX3203_REG_ALS_HIGH_INT_THD_17_16_AND_3_0 0x10
#define HX3203_REG_ALS_LOW_INT_THD_17_16_AND_3_0 0x11
#define HX3203_REG_ALS_RES 0x26

#define LOBYTE(w) ((unsigned char)(w))
#define HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))


struct hx3203_config {
	struct i2c_dt_spec i2c;
};


struct hx3203_data {
	struct k_sem sem;
	uint16_t light;
};



#ifdef __cplusplus
}
#endif


#endif /* _HX3203_H_ */