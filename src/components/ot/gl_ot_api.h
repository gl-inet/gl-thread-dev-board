/*****************************************************************************
 * @file  gl_ot_api.h
 * @brief The header file of gl_ot_api.c
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

#ifndef _GL_OT_API_H_
#define _GL_OT_API_H_

enum {
	OT_SETTINGS_KEY_BASE_START = 0x8000,
	OT_SETTINGS_KEY_MARRIED = OT_SETTINGS_KEY_BASE_START,
	OT_SETTINGS_KEY_BASE_END = 0xffff
};

char *ot_get_eui64(void);
char *ot_get_extaddr(void);
int ot_get_thread_version(void);
const char *ot_get_version(void);
int ot_get_rloc16(void);
char *ot_get_mleid(void);
char *ot_get_slaac_addr(void);
char *ot_get_device_type(void);
void ot_factoryreset(void);
int ot_start(void);
void ot_print_network_info(void);
char *ot_get_mode(void);
int ot_get_txpower(void);

void start_joiner(void);

#endif /* _GL_OT_API_H_ */