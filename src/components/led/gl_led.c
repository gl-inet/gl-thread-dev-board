/*****************************************************************************
 * @file  gl_led.c
 * @brief Provide led related functions
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

#include "gl_led.h"

static struct k_timer led_timer;
static bool led_timer_start;
static int light_state;


static void on_led_timer_expiry(struct k_timer *timer_id)
{
	static uint32_t val = 1;

	ARG_UNUSED(timer_id);

	dk_set_led((uint8_t)LED2, val);
	val = !val;
}

static void on_led_timer_stop(struct k_timer *timer_id)
{
	ARG_UNUSED(timer_id);

	dk_set_led((uint8_t)LED2, (uint32_t)LED_OFF);
}

void led_toggle_start(int ms)
{
	k_timer_stop(&led_timer);

	led_timer_start = true;

	k_timer_init(&led_timer, on_led_timer_expiry, on_led_timer_stop);
	k_timer_start(&led_timer, K_MSEC(100), K_MSEC(ms));
}

void led_toggle_stop(void)
{
	k_timer_stop(&led_timer);

	led_timer_start = false;
}

void led_on(uint8_t led_idx)
{
	if (led_timer_start)
		led_toggle_stop();

	dk_set_led_on(led_idx);
}

void led_off(uint8_t led_idx)
{
	if (led_timer_start)
		led_toggle_stop();

	dk_set_led_off(led_idx);
}

void light_on(void)
{
	dk_set_led_on(LED1);
	light_state = 1;
}

void light_off(void)
{
	dk_set_led_off(LED1);
	dk_set_led_off(LED2);
	light_state = 0;
}

void light_set_state(int val)
{
	dk_set_led(LED1, val);
	light_state = val;
}

int light_get_state(void)
{
	return light_state;
}

void light_toggle_onoff(void)
{
	light_state = !light_state;
	light_set_state(light_state);
}

void light_onoff(void)
{
	light_set_state(0);
	k_sleep(K_MSEC(100));
	light_set_state(1);
	k_sleep(K_MSEC(100));
	light_set_state(0);
}