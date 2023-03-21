/*****************************************************************************
 * @file  gl_led.h
 * @brief The header file of gl_led.c
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

#ifndef _GL_LED_H_
#define _GL_LED_H_

/**
 * @brief OnOff Light
 * Control by Button1
 */
#define LED1 0
#define LED2 1

/**
 * @brief Thread State
 * 1.Initial: 
 * 2.Connected:
 * 3.Disconnected: 
 * 4.Joining
 */

enum{
	LED_OFF		= 0,
	LED_ON 		= 1,
	LED_TOGGLE	= 2
};

#define BUTTON_S1
#define BUTTON_S2

void led_toggle_start(int ms);
void led_toggle_stop(void);
void led_on(uint8_t led_idx);
void led_off(uint8_t led_idx);

void light_on(void);
void light_off(void);
void light_set_state(int val);
int light_get_state(void);
void light_toggle_onoff(void);
void light_onoff(void);

#endif /* _GL_LED_H_ */