/*****************************************************************************
 * @file  gl_srp_utils.h
 * @brief The header file of gl_srp_utils.c
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

#ifndef _GL_SRP_UTILS_H_
#define _GL_SRP_UTILS_H_

#include <openthread/srp_client.h>

typedef void (*srp_srv_reg_cb)(void);


int srp_utils_set_host_name(char *host_name);
int srp_utils_set_host_address(char *host_address);
int srp_utils_add_service(char *instance_name, char *service_name, uint16_t port);
void srp_utils_autostart(srp_srv_reg_cb cb);
int srp_utils_host_remove(void);

#endif /* _GL_SRP_UTILS_H_ */