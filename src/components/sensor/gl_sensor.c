/*****************************************************************************
 * @file  gl_sensor.c
 * @brief Provide sensor related functions.
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

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include "gl_sensor.h"

LOG_MODULE_REGISTER(gl_sensor, CONFIG_GL_SENSOR_LOG_LEVEL);

#ifdef CONFIG_SHTCX
const struct device *sensor_shtcx = DEVICE_DT_GET_ONE(sensirion_shtcx); //温湿度传感器:温度、湿度
#endif
#ifdef CONFIG_HX3203
const struct device *sensor_hx3203 = DEVICE_DT_GET_ONE(tianyihexin_hx3203); //光照传感器:光照强度
#endif
#ifdef CONFIG_SPL0601
const struct device *sensor_spl0601 = DEVICE_DT_GET_ONE(goertek_spl0601); //气压传感器:气压
#endif

void gl_sensor_init(void)
{
#ifdef CONFIG_SHTCX
	if (!device_is_ready(sensor_shtcx)) {
		printf("Device %s is not ready\n", sensor_shtcx->name);
		return;
	}
#endif

#ifdef CONFIG_HX3203
	// sensor_hx3203 = device_get_binding("HX3203");
	// if (sensor_hx3203 == NULL) {
	// 	printk("Could not get HX3203 device");
	// 	return;
	// }
	if (!device_is_ready(sensor_hx3203)) {
		printf("Device %s is not ready\n", sensor_hx3203->name);
		return;
	}
#endif

#ifdef CONFIG_SPL0601
	if (!device_is_ready(sensor_spl0601)) {
		printf("Device %s is not ready\n", sensor_spl0601->name);
		return;
	}
#endif
}

void gl_sensor_sample_fetch(void)
{
	int rc;
#ifdef CONFIG_SHTCX
	rc = sensor_sample_fetch(sensor_shtcx);
	if (rc != 0) {
		printk("sensor_sample_fetch sensor_shtcx failed: %d\n", rc);
	}
#endif

#ifdef CONFIG_HX3203
	rc = sensor_sample_fetch(sensor_hx3203);
	if (rc != 0) {
		printk("sensor_sample_fetch sensor_hx3203 failed: %d\n", rc);
	}
#endif

#ifdef CONFIG_SPL0601
	rc = sensor_sample_fetch(sensor_spl0601);
	if (rc != 0) {
		printk("sensor_sample_fetch sensor_spl0601 failed: %d\n", rc);
	}
#endif
}

double gl_sensor_get_temp(void)
{
	int rc;
	struct sensor_value temp;
#ifdef CONFIG_SHTCX
	rc = sensor_channel_get(sensor_shtcx, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	if (rc != 0) {
		printk("SHT3XD: failed: %d\n", rc);
		return 0;
	}
#endif
	return sensor_value_to_double(&temp);
}

double gl_sensor_get_humi(void)
{
	int rc;
	struct sensor_value humi;
#ifdef CONFIG_SHTCX
	rc = sensor_channel_get(sensor_shtcx, SENSOR_CHAN_HUMIDITY, &humi);
	if (rc != 0) {
		printk("SHT3XD: failed: %d\n", rc);
		return 0;
	}
#endif
	return sensor_value_to_double(&humi);
}

double gl_sensor_get_light(void)
{
	int rc;
	struct sensor_value light;
#ifdef CONFIG_HX3203
	rc = sensor_channel_get(sensor_hx3203, SENSOR_CHAN_LIGHT, &light);
	if (rc != 0) {
		printk("sensor_hx3203 failed: %d\n", rc);
		return 0;
	}
#endif
	return sensor_value_to_double(&light);
}
double gl_sensor_get_press(void)
{
	int rc;
	struct sensor_value press;
#ifdef CONFIG_SPL0601
	rc = sensor_channel_get(sensor_spl0601, SENSOR_CHAN_PRESS, &press);
	if (rc != 0) {
		printk("sensor_spl0601 failed: %d\n", rc);
		return 0;
	}
#endif
	return sensor_value_to_double(&press);
}

double gl_sensor_get_temp_spl0601(void)
{
	int rc;
	struct sensor_value temp;
#ifdef CONFIG_SPL0601
	rc = sensor_channel_get(sensor_spl0601, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	if (rc != 0) {
		printk("sensor_spl0601: failed: %d\n", rc);
		return 0;
	}
#endif
	return sensor_value_to_double(&temp);
}



#ifdef CONFIG_SENSOR_VALUE_AUTO_PRINT
static K_THREAD_STACK_DEFINE(test_sensor_area, 1024);
static struct k_thread test_sensor_thread_data;

static void test_sensor(void)
{
	double temp = 0;
	double temp_spl0601 = 0;
	double humi = 0;
	double light = 0;
	double press = 0;
	while (1) {
		gl_sensor_sample_fetch();

		temp = gl_sensor_get_temp();
		humi = gl_sensor_get_humi();
		light = gl_sensor_get_light();
		temp_spl0601 = gl_sensor_get_temp_spl0601();
		press = gl_sensor_get_press();

		printf("Temp: %.2f Cel ; Humi: %0.2f %%RH\n", temp, humi);
		printf("Light: %.2f Cd ; Press: %0.2f %%kPa\n", light, press);
		printf("SPL0601 Temp:  %.2f Cel\n", temp_spl0601);
		printf("\n");

		k_sleep(K_MSEC(1000));
	}
}

void debug_sensor_data(void)
{
	k_thread_create(&test_sensor_thread_data, test_sensor_area,
			K_THREAD_STACK_SIZEOF(test_sensor_area), (k_thread_entry_t)test_sensor,
			NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	k_thread_name_set(&test_sensor_thread_data, "TEST-sensor-thread");

	return;
}
#endif 

