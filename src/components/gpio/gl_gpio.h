/*****************************************************************************
 * @file  gl_gpio.h
 * @brief The header file of gl_gpio.c
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

#ifndef _GL_GPIO_H_
#define _GL_GPIO_H_

typedef enum {
    GPIO_LOW    = 0,
    GPIO_HIGH   = 1
} gl_gpio_status_e;

typedef enum { 
    GPIO_015 = 0,
    GPIO_016 = 1,
    GPIO_017 = 2,
    GPIO_020 = 3,
    GPIO_NULL = 0xff,
} gl_gpio_node_e;

int gl_gpio_init(void);

gl_gpio_status_e gl_get_gpio_status(gl_gpio_node_e node);

char* gl_get_gpio_name(gl_gpio_node_e node);

int gl_set_gpio_status_by_id(gl_gpio_node_e node_id, gl_gpio_status_e status);

int gl_set_gpio_status_by_name(char* node_name, gl_gpio_status_e status);

#endif /* _GL_GPIO_H_ */