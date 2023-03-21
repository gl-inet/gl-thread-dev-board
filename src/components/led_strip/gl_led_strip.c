/*****************************************************************************
 * @file  gl_led_strip.c
 * @brief Provide RGB led strip related functions
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
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <string.h>

#include "gl_led_strip.h"

#define STRIP_NODE DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)

#define DELAY_TIME K_MSEC(500)

static bool strip_on_off[STRIP_NUM_PIXELS];
static struct led_rgb pixels[STRIP_NUM_PIXELS];
static struct led_rgb real_pixels[STRIP_NUM_PIXELS];

static const struct led_rgb def_colors[] = {
	RGB(0xff, 0x00, 0x00), /* red */
	RGB(0xff, 0x80, 0x00), /* orange */
	RGB(0xff, 0xff, 0x00), /* yellow */
	RGB(0x00, 0xff, 0x00), /* green */
	RGB(0x00, 0x00, 0xff), /* blue */
	RGB(0x08, 0x2e, 0x54), /* indigo */
	RGB(0x8a, 0x28, 0xe2), /* purple */
	RGB(0xff, 0xff, 0xff), /* white */
};

const struct led_rgb def_on = { .r = (0xff), .g = (0xff), .b = (0xff) };
const struct led_rgb def_off = { .r = (0x00), .g = (0x00), .b = (0x00) };

static const struct device *strip = DEVICE_DT_GET(STRIP_NODE);

static struct k_timer delay_timer;
static bool delay_timer_is_work = false;
static void led_delay_timer_expiry(struct k_timer *timer_id);
static void led_delay_work(struct k_work *item);
static uint16_t delay_node; 
static int delay_on_off;
static struct k_work delay_work;


static int led_update_rgb(void);

enum{
	LED_OFF		= 0,
	LED_ON 		= 1,
	LED_TOGGLE	= 2
};

int gl_led_strip_init(void)
{
	if (device_is_ready(strip)) {
		printk("Found LED strip device %s\n", strip->name);
	} else {
		printk("LED strip device %s is not ready\n", strip->name);
		return -1;
	}

	for(int i = 0; i < STRIP_NUM_PIXELS; i++)
	{
		strip_on_off[i] = false;
		memcpy(&pixels[i], &def_off, sizeof(struct led_rgb));
	}

	int rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
	if (rc) {
		printk("couldn't update strip: %d\n", rc);
		return -2;
	}else{
		printk("Update strip successful\n");
	}

	k_work_init(&delay_work, led_delay_work);
	
	return 0;
}

static int led_update_rgb(void)
{
	for(int i = 0; i < STRIP_NUM_PIXELS; i++)
	{
		memcpy(&real_pixels[i], &pixels[i], sizeof(struct led_rgb));
		if(strip_on_off[i])
		{
			// Set to white when the original value is off, but currently needs to be turned on.
			// This only occurs when first set on.
			if(0 == memcmp(&real_pixels[i], &def_off, sizeof(struct led_rgb)))
			{
				memcpy(&pixels[i], &def_on, sizeof(struct led_rgb));
				memcpy(&real_pixels[i], &def_on, sizeof(struct led_rgb));
			}
		}else{
			// When the original value is on but currently needs to be turned off, set to off, 
			// but save the previously set color.
			if(0 != memcmp(&real_pixels[i], &def_off, sizeof(struct led_rgb)))
			{
				memcpy(&real_pixels[i], &def_off, sizeof(struct led_rgb));
			}
		}
	}

	int rc = led_strip_update_rgb(strip, real_pixels, STRIP_NUM_PIXELS);
	if (rc) {
		printk("couldn't update strip: %d\n", rc);
		return -2;
	}else{
		printk("Update strip successful\n");
	}

	return 0;
}



int update_led_strip_rgb(uint16_t node, struct led_rgb* color)
{
	if (node > STRIP_NUM_PIXELS) {
		printk("node is not exist!\n");
		return -1;
	}

	printk("node: %d \nr: %d g: %d b: %d\n", node, color->r, color->g, color->b);

	if(node == ALL_LED_NODE)
	{
		for(int i = 0; i < STRIP_NUM_PIXELS; i++)
		{
			memcpy(&pixels[i], color, sizeof(struct led_rgb));
		}
	} else {
		memcpy(&pixels[(node - 1)], color, sizeof(struct led_rgb));
		if (false == strip_on_off[(node - 1)]) {
			return -3;
		}
	}

	return led_update_rgb();
}

int on_off_led_strip(uint16_t node, int on_off)
{
	if (node > STRIP_NUM_PIXELS) {
		printk("node is not exist!\n");
		return -1;
	}

	bool led_status;
	if(on_off == LED_ON)
	{
		led_status = true;
	}else if(on_off == LED_OFF) {
		led_status = false;
	}

	if(node == ALL_LED_NODE)
	{
		for(int i = 0; i < STRIP_NUM_PIXELS; i++)
		{
			if(on_off == LED_TOGGLE)
			{
				led_status = !strip_on_off[i];
			}

			strip_on_off[i] = led_status;
		}	
	} else {
		if(on_off == LED_TOGGLE)
		{
			led_status = !strip_on_off[(node - 1)];
		}

		strip_on_off[(node - 1)] = led_status;
	}

	return led_update_rgb();
}

int on_off_led_strip_with_delay(uint16_t node, int on_off, uint16_t delay_ms)
{
	if (node > STRIP_NUM_PIXELS) {
		printk("node is not exist!\n");
		return -1;
	}

	if(delay_timer_is_work)
	{
		k_timer_stop(&delay_timer);
		delay_timer_is_work = false;
	}

	delay_node = node;
	delay_on_off = on_off;

	delay_timer_is_work = true;

	k_timer_init(&delay_timer, led_delay_timer_expiry, NULL);

	k_timer_start(&delay_timer, K_MSEC(delay_ms), K_MSEC(0));

	return 0;
}

static void led_delay_timer_expiry(struct k_timer *timer_id)
{
	ARG_UNUSED(timer_id);

	k_work_submit(&delay_work);
}

static void led_delay_work(struct k_work *item)
{
	ARG_UNUSED(item);

	on_off_led_strip(delay_node, delay_on_off);
}

int get_led_strip_status(uint16_t node, int* on_off, struct led_rgb* color)
{
	if ((node > STRIP_NUM_PIXELS) || (node == ALL_LED_NODE)) {
		printk("node is not exist!\n");
		return -1;
	}

	if(strip_on_off[(node - 1)] == true)
	{
		*on_off = 1;
	}else{
		*on_off = 0;
	}

	memcpy(color, &pixels[(node - 1)], sizeof(struct led_rgb));

	return 0;
}



/* Automatic continuous color change */
void test_led_strip_1(void)
{
	size_t cursor = 0, color = 0;
	int rc;

	while (1) {
		memset(&pixels, 0x00, sizeof(pixels));
		memcpy(&pixels[cursor], &def_colors[color], sizeof(struct led_rgb));
		rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

		if (rc) {
			printk("couldn't update strip: %d\n", rc);
		}else{
			printk("Update strip successful - rc: %d\n", rc);
		}


		cursor++;
		if (cursor >= STRIP_NUM_PIXELS) {
			cursor = 0;
			color++;
			if (color == ARRAY_SIZE(def_colors)) {
				color = 0;
			}
		}
		

		k_sleep(DELAY_TIME);
	}
}

void test_led_strip_2(void)
{
	static size_t color = 0;

	color++;
	if (color == ARRAY_SIZE(def_colors)) {
		color = 0;
	}

	int i = 0;
	for(i = 0; i < STRIP_NUM_PIXELS; i++)
	{
		memcpy(&pixels[i], &def_colors[color], sizeof(struct led_rgb));
	}

	int rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
	if (rc) {
		printk("couldn't update strip: %d\n", rc);
		return;
	}

	return;
}
