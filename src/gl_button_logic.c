/*****************************************************************************
 * @file  gl_button_logic.c
 * @brief Provides button related functions.
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
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <openthread/thread.h>

#include "gl_types.h"
#include "gl_led.h"
#include "gl_coap.h"
#include "gl_ot_api.h"
#include "gl_srp_utils.h"

LOG_MODULE_REGISTER(button_logic, CONFIG_GL_THREAD_DEV_BOARD_LOG_LEVEL);

#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
const struct gpio_dt_spec SW1 = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, { 0 });
const struct gpio_dt_spec SW2 = GPIO_DT_SPEC_GET_OR(SW1_NODE, gpios, { 0 });

extern int joiner_state;
uint32_t join_time_start;


void on_button_changed(uint32_t button_state, uint32_t has_changed)
{
	static bool is_sw2_press, is_sw2_release;
	static uint32_t sw2_time_start;
	uint32_t sw2_time_end = 0;

	static bool is_sw1_press, is_sw1_release;
	static uint32_t sw1_time_start;
	uint32_t sw1_time_end = 0;

	uint32_t buttons = button_state & has_changed;

	if(buttons & DK_BTN1_MSK) { // Press
		is_sw1_press = true;
		sw1_time_start = k_uptime_get_32();
	}else if (buttons & DK_BTN2_MSK) { // Press
		is_sw2_press = true;
		sw2_time_start = k_uptime_get_32();
	} 

	if (is_sw1_press) {
		if (gpio_pin_get_dt(&SW1) == 0) { // Release
			is_sw1_release = true;
			sw1_time_end = k_uptime_get_32();
		}
	}

	if (is_sw2_press) {
		if (gpio_pin_get_dt(&SW2) == 0) { // Release
			is_sw2_release = true;
			sw2_time_end = k_uptime_get_32();
		}
	}

	if (is_sw1_press && is_sw1_release) {
		printk("Button1\n");
		if ((sw1_time_end - sw1_time_start) < 300) { // update the status
			LOG_INF("Updating status...");
			coap_client_send_status();
			ot_print_network_info();
		} else { // Testing mode
			LOG_INF("Start testing mode...");
			start_testing_mode();
		}
		is_sw1_press = false;
		is_sw1_release = false;

	}else if (is_sw2_press && is_sw2_release) {
		printk("Button2\n");
		if ((sw2_time_end - sw2_time_start) < 300) { // Joining
			join_time_start = k_uptime_get_32();
			if (joiner_state != DEVICE_CONNECTED) {
				if (ot_start() == 0) {
					joiner_state = DEVICE_CONNECTING;
					led_toggle_start(200);
					LOG_INF("Joining start...");
				}
			}
			is_sw2_press = false;
			is_sw2_release = false;
		} else if ((sw2_time_end - sw2_time_start) > 3000) { // Factoryreset
			LOG_INF("Factoryreset start...");
#ifdef CONFIG_OPENTHREAD_SRP_CLIENT
			srp_utils_host_remove();
#endif
			k_sleep(K_MSEC(1000));
			ot_factoryreset();
		}
	} else if (buttons & DK_BTN3_MSK) {
		printk("Button3 Press\n");
		send_trigger_event_request(INFRARED_SENSOR_TRIGGER, "infra_0", "trigger");

	} else if (buttons & DK_BTN4_MSK) {
		printk("Button4 Press\n");

		send_trigger_event_request(QDEC_BUTTON_TRIGGER, "qdec_0", "trigger");
	}
}
