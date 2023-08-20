/*****************************************************************************
 * @file  gl_led_strip.h
 * @brief The header file of gl_led_strip.c
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

#ifndef _GL_LED_STRIP_H_
#define _GL_LED_STRIP_H_

#include <zephyr/drivers/led_strip.h>

#define ALL_LED_NODE        0x0
#define LED_STRIP_NODE_1    0x1
#define LED_STRIP_NODE_2    0x2

#define RGB(_r, _g, _b)                                                                   \
	{                                                                                     \
		.r = (_r), .g = (_g), .b = (_b)                                                   \
	}



int gl_led_strip_init(void);

int update_led_strip_rgb_to_next(void);

int update_led_strip_rgb(uint16_t node, struct led_rgb* color);

int on_off_led_strip(uint16_t node, int on_off);

int on_off_led_strip_with_delay(uint16_t node, int on_off, uint16_t delay_ms);

int get_led_strip_status(uint16_t node, int* on_off, struct led_rgb* color);

void test_led_strip_1(void);
void test_led_strip_2(void);


#endif
