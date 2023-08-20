/*****************************************************************************
 * @file  gl_coap.h
 * @brief The header file of gl_coap.c
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

#ifndef _GL_COAP_CLIENT_H_
#define _GL_COAP_CLIENT_H_

#include <stdbool.h>
#include <cJSON.h>
#include <cJSON_os.h>

#define COAP_PORT 5683


#define PROVISIONING_URI_PATH "provisioning"
#define STATUS_URI_PATH "status"
#define TRIGGER_REPO_URI_PATH "trigger"

#define TESTING_LIGHT_URI_PATH "testing_light"

typedef enum{
	INFRARED_SENSOR_TRIGGER = 1,
	QDEC_BUTTON_TRIGGER		= 2,
	QDEC_ROTATE_TRIGGER		= 3
}trigger_event_type_e;

/** @brief Type indicates function called when OpenThread connection
 *         is established.
 *
 * @param[in] item pointer to work item.
 */
typedef void (*ot_connection_cb_t)(struct k_work *item);

/** @brief Type indicates function called when OpenThread connection is ended.
 *
 * @param[in] item pointer to work item.
 */
typedef void (*ot_disconnection_cb_t)(struct k_work *item);

/** @brief Type indicates function called when the MTD modes are toggled.
 *
 * @param[in] val 1 if the MTD is in MED mode
 *                0 if the MTD is in SED mode
 */
typedef void (*mtd_mode_toggle_cb_t)(uint32_t val);

/** @brief Initialize CoAP client utilities.
 */
void coap_client_utils_init(ot_connection_cb_t on_connect, ot_disconnection_cb_t on_disconnect,
			    mtd_mode_toggle_cb_t on_toggle);



/** @brief Request for the CoAP server address to pair.
 *
 * @note Enable paring on the CoAP server to get the address.
 */
void coap_client_send_provisioning_request(void);

/** @brief Toggle SED to MED and MED to SED modes.
 *
 * @note Active when the device is working as Minimal Thread Device.
 */
void coap_client_toggle_minimal_sleepy_end_device(void);

void coap_client_send_status(void);

typedef int (*cmd_request_callback_t)(const char *json_str, cJSON* resp_obj);

void send_trigger_event_request(trigger_event_type_e event, char* obj, void* value);

void start_testing_mode(void);


#endif /* _GL_COAP_CLIENT_H_ */