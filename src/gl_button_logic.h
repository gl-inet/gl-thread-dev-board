/*****************************************************************************
 * @file  gl_button_logic.h
 * @brief The header file of gl_button_logic.c
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

#ifndef _GL_BUTTON_H_
#define _GL_BUTTON_H_

#include <zephyr/types.h>


void on_button_changed(uint32_t button_state, uint32_t has_changed);


#endif /* _GL_BUTTON_H_ */