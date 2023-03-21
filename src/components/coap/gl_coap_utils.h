/*****************************************************************************
 * @file  gl_coap_utils.h
 * @brief The header file of gl_coap_utils.c
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

#ifndef _GL_COAP_UTILS_H_
#define _GL_COAP_UTILS_H_

#define MAX_COAP_MSG_LEN 512
#define COAP_VER 1
#define COAP_TOKEN_LEN 8
#define COAP_MAX_REPLIES 1
#define COAP_POOL_SLEEP 500
#define COAP_OPEN_SOCKET_SLEEP 200
#if defined(CONFIG_NRF_MODEM_LIB)
#define COAP_RECEIVE_STACK_SIZE 1096
#else
#define COAP_RECEIVE_STACK_SIZE 996
#endif

#endif /* _GL_COAP_UTILS_H_ */