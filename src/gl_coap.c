/*****************************************************************************
 * @file  gl_coap.c
 * @brief Provide coap related functions.
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

#include <stdio.h>
#include <string.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/kernel.h>
#include <net/coap_utils.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/socket.h>
#include <openthread/thread.h>
#include <openthread/message.h>
#include <openthread/coap.h>
#include <openthread/joiner.h>
#include <dk_buttons_and_leds.h>

#include "gl_cjson_utils.h"
#include "gl_coap.h"
#include "gl_srp_utils.h"
#include "gl_types.h"
#include "gl_ot_api.h"
#include "gl_sensor.h"
#include "gl_led.h"
#include "gl_led_strip.h"
#include "gl_gpio.h"
#include "gl_battery.h"

LOG_MODULE_REGISTER(gl_coap, CONFIG_GL_THREAD_DEV_BOARD_LOG_LEVEL);

#define RESPONSE_POLL_PERIOD 100

#define CONFIG_DEFAULT_REPORT_AFTER (1 * 60 * 1000)
#define CONFIG_DEFAULT_REPORT_REPEAT (5 * 60 * 1000)
#define JOIN_COMMISSIONING_TIMEOUT	(3 * 60 * 1000)

struct _cmd_s {
	int id;
	char cmd[64];
} g_cmd[] = {
	{ CONFIG_CMD_ON_OFF, "onoff" },
	{ CONFIG_CMD_UPGRADE, "upgrade" },
	{ CONFIG_CMD_FACTORYRESET, "factoryreset" },
	{ CONFIG_CMD_REBOOT, "reboot" },
	{ CONFIG_CMD_CHANGE_COLOR, "change_color" },
	{ CONFIG_CMD_SET_GPIO, "set_gpio" },
	{ CONFIG_CMD_GET_GPIO_STATUS, "get_gpio_status" },
	{ CONFIG_CMD_GET_LED_STATUS, "get_led_status" },
	{ CONFIG_CMD_SET_REPORT_INTERVAL, "set_report_interval" },
	{ CONFIG_CMD_SET_OT_MODE, "set_ot_mode"},
};

struct _obj_s {
	int id;
	char obj[64];
} g_led_obj[] = {
	{ CONFIG_OBJ_LED_STRIP_NODE_ALL, "all" },
	{ CONFIG_OBJ_LED_STRIP_NODE_LEFT, "led_left" },
	{ CONFIG_OBJ_LED_STRIP_NODE_RIGHT, "led_right" },
};

static uint32_t poll_period;

static bool is_joined;
static bool is_connected;
static bool is_srp_client_running = false;

static struct k_work multicast_light_work;
static struct k_work toggle_MTD_SED_work;
static struct k_work provisioning_work;
static struct k_work on_connect_work;
static struct k_work on_disconnect_work;
static struct k_work report_status_work;
static struct k_work factory_reset_work;
// static struct k_timer factory_reset_timer;

static struct k_timer report_timer;

mtd_mode_toggle_cb_t on_mtd_mode_toggle;
extern uint32_t join_time_start;
int report_interval_second = CONFIG_DEFAULT_REPORT_REPEAT/1000;  //millisecond to second

/* Options supported by the server */
static const char *const provisioning_option[] = { PROVISIONING_URI_PATH, NULL };
static const char *const status_option[] = { STATUS_URI_PATH, NULL };
static const char *const trigger_repo_option[] = { TRIGGER_REPO_URI_PATH, NULL };

static const char *const testing_light_option[] = { TESTING_LIGHT_URI_PATH, NULL };

struct server_context {
	struct otInstance *ot;
	cmd_request_callback_t cmd_request;
};

static struct server_context srv_context = {
	.ot = NULL,
	.cmd_request = NULL,
};

/**@brief Definition of CoAP resources for light. */
static otCoapResource cmd_resource = {
	.mUriPath = "cmd",
	.mHandler = NULL,
	.mContext = NULL,
	.mNext = NULL,
};

/**@brief Definition of CoAP resources for testing mode. */
static otCoapResource testing_light_resource = {
	.mUriPath = TESTING_LIGHT_URI_PATH,
	.mHandler = NULL,
	.mContext = NULL,
	.mNext = NULL,
};

/* Thread multicast mesh local address */
static struct sockaddr_in6 multicast_local_addr = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(COAP_PORT),
	.sin6_addr.s6_addr = { 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00, 0x00, 0x00, 0x01 },
	.sin6_scope_id = 0U
};

/* Variable for storing server address acquiring in provisioning handshake */
static char unique_local_addr_str[INET6_ADDRSTRLEN];
static struct sockaddr_in6 unique_local_addr = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(COAP_PORT),
	.sin6_addr.s6_addr = {0, },
	.sin6_scope_id = 0U
};


/********************************************************************************************
 * 									Testing Mode
********************************************************************************************/
static bool testing_mode_flag = false;
void start_testing_mode(void)
{
	if(is_connected)
	{
		if(!testing_mode_flag)
		{
			testing_mode_flag = true;
			k_timer_stop(&report_timer);
		}
	}else{
		LOG_ERR("No network connected! Starting testing mode failed!");
		return;
	}
}

static bool is_testing_mode(void)
{
	return testing_mode_flag;
}

static void testing_light_request_handler(void *context, otMessage *message,
				const otMessageInfo *message_info)
{
	if(!is_testing_mode())
	{
		return;
	}

	char buf[150];
	int length;

	ARG_UNUSED(context);

	if (otCoapMessageGetType(message) != OT_COAP_TYPE_NON_CONFIRMABLE) {
		LOG_ERR("Testing Light handler - Unexpected type of message");
		return;
	}

	if (otCoapMessageGetCode(message) != OT_COAP_CODE_PUT) {
		LOG_ERR("Testing Light handler - Unexpected CoAP code");
		return;
	}
	length = otMessageRead(message, otMessageGetOffset(message), buf, sizeof(buf) - 1);
	buf[length] = '\0';

	LOG_INF("Received Testing Light request: %s", buf);
/*
	otError error;
	otMessageInfo msg_info;
	msg_info = *message_info;

	char* resp;
	int resp_len;

	cJSON *resp_obj = cJSON_CreateObject();
	ret = srv_context.cmd_request(buf, resp_obj);

	resp = cJSON_PrintUnformatted(resp_obj);

	error = coap_send_utils(message, &msg_info, resp, strlen(resp));
	if (error != OT_ERROR_NONE) {
		LOG_INF("coap_send_utils failed. error = %d", error);
	}

	free(resp);
	cJSON_Delete(resp_obj);
*/
	cJSON *root_obj = NULL;
	root_obj = cJSON_Parse(buf);
	if (root_obj == NULL) {
		LOG_ERR("cJSON Parse failure");
		return;
	}

	cJSON* event_obj = cJSON_GetObjectItem(root_obj, "event");

	char* trigger_type = gl_json_get_string(event_obj, "trigger_type");
	if(trigger_type == NULL)
	{
		LOG_ERR("get trigger_type failure");
		return;
	}

	if(0 == strcmp(trigger_type, "qdec_button"))
	{
		on_off_led_strip(ALL_LED_NODE, LED_TOGGLE);
	}else if(0 == strcmp(trigger_type, "qdec_rotate")) {
		// if(0 > gl_json_get_int(event_obj, "value"))
		// {
			
		// }else{

		// }
		update_led_strip_rgb_to_next();
	}

	return;
}

/********************************************************************************************/
#if 0
static bool check_cmd_valid(const char *cmd)
{
	if (!cmd)
		return false;

	for (int i = 0; i < ARRAY_SIZE(g_cmd); i++) {
		if (!strcmp(g_cmd[i].cmd, cmd))
			return true;
	}
	return false;
}
#endif

void do_after_srp_srv_reg(void);

static int get_cmd_id(const char *cmd)
{
	if (!cmd)
		return -1;

	for (int i = 0; i < ARRAY_SIZE(g_cmd); i++) {
		if (!strcmp(g_cmd[i].cmd, cmd))
			return g_cmd[i].id;
	}

	return -1;
}

static bool check_obj_valid(const char *obj)
{
	if (!obj)
		return false;

	for (int i = 0; i < ARRAY_SIZE(g_led_obj); i++) {
		if (!strcmp(g_led_obj[i].obj, obj))
			return true;
	}
	return false;
}

static bool is_mtd_in_med_mode(otInstance *instance)
{
	return otThreadGetLinkMode(instance).mRxOnWhenIdle;
}

static void poll_period_response_set(void)
{
	otError error;

	otInstance *instance = openthread_get_default_instance();

	if (is_mtd_in_med_mode(instance)) {
		return;
	}

	if (!poll_period) {
		poll_period = otLinkGetPollPeriod(instance);

		error = otLinkSetPollPeriod(instance, RESPONSE_POLL_PERIOD);
		__ASSERT(error == OT_ERROR_NONE, "Failed to set pool period");

		LOG_INF("Poll Period: %dms set", RESPONSE_POLL_PERIOD);
	}
}

static void poll_period_restore(void)
{
	otError error;
	otInstance *instance = openthread_get_default_instance();

	if (is_mtd_in_med_mode(instance)) {
		return;
	}

	if (poll_period) {
		error = otLinkSetPollPeriod(instance, poll_period);
		__ASSERT_NO_MSG(error == OT_ERROR_NONE);

		LOG_INF("Poll Period: %dms restored", poll_period);
		poll_period = 0;
	}
}

static void initial_unique_local_addr(void)
{
	struct otInstance *instance = openthread_get_default_instance();
	const otSockAddr *serverSockAddr = otSrpClientGetServerAddress(instance);
	memcpy(&unique_local_addr.sin6_addr, &serverSockAddr->mAddress, sizeof(unique_local_addr.sin6_addr));

	inet_ntop(AF_INET6, &(unique_local_addr.sin6_addr), unique_local_addr_str, INET6_ADDRSTRLEN);
}

static int on_provisioning_reply(const struct coap_packet *response, struct coap_reply *reply,
				 const struct sockaddr *from)
{
	int ret = 0;
	const uint8_t *payload;
	uint16_t payload_size = 0u;

	ARG_UNUSED(reply);
	ARG_UNUSED(from);

	payload = coap_packet_get_payload(response, &payload_size);

	if (payload == NULL || payload_size != sizeof(unique_local_addr.sin6_addr)) {
		LOG_ERR("Received data is invalid");
		ret = -EINVAL;
		goto exit;
	}

	memcpy(&unique_local_addr.sin6_addr, payload, payload_size);

	if (!inet_ntop(AF_INET6, payload, unique_local_addr_str, INET6_ADDRSTRLEN)) {
		LOG_ERR("Received data is not IPv6 address: %d", errno);
		ret = -errno;
		goto exit;
	}

	LOG_INF("Received peer address: %s", unique_local_addr_str);

	coap_client_send_status();

exit:
	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
		poll_period_restore();
	}

	return ret;
}

struct trigger{
	trigger_event_type_e 	event_id;
	char*					event_name[64];
} g_trigger_event[] = {
	{INFRARED_SENSOR_TRIGGER, "infrared_sensor"},
	{QDEC_BUTTON_TRIGGER, "qdec_button"},
	{QDEC_ROTATE_TRIGGER, "qdec_rotate"}
};

static int on_send_trigger_reply(const struct coap_packet *response, struct coap_reply *reply,
				 const struct sockaddr *from)
{
	ARG_UNUSED(response);
	ARG_UNUSED(reply);
	ARG_UNUSED(from);

	LOG_INF("Send 'trigger' done.");
	return 0;
}

void send_trigger_event_request(trigger_event_type_e event, char* obj, void* value)
{
	if((NULL == obj) || (value == NULL))
	{
		return;
	}

	if (!is_connected)
	{
		LOG_WRN("device does not connect!");
		return;
	}

	if(!is_testing_mode())
	{
		if (unique_local_addr.sin6_addr.s6_addr16[0] == 0) {
			LOG_WRN("Peer address not set");
			coap_client_send_provisioning_request();
			return;
		}
	}

	
	cJSON *root_obj = cJSON_CreateObject();
	gl_json_add_str(root_obj, "eui64", ot_get_eui64());

	cJSON *event_obj = cJSON_CreateObject();
	switch(event) {
		case INFRARED_SENSOR_TRIGGER:
		{
			gl_json_add_str(event_obj, "trigger_type", "infrared_sensor");
			gl_json_add_str(event_obj, "obj", obj);
			// int* p = (int*)value;
			// gl_json_add_boolean(event_obj, "value", *p);
		} break;
		case QDEC_BUTTON_TRIGGER:
		{
			gl_json_add_str(event_obj, "trigger_type", "qdec_button");
			gl_json_add_str(event_obj, "obj", obj);
			// int* p = (int*)value;
			// gl_json_add_boolean(event_obj, "value", *p);
		} break;
		case QDEC_ROTATE_TRIGGER:
		{
			gl_json_add_str(event_obj, "trigger_type", "qdec_rotate");
			gl_json_add_str(event_obj, "obj", obj);
			double* p = (double*)value;
			gl_json_add_number(event_obj, "value", *p);
		} break;
		default:
			LOG_ERR("Unknow trgger event: %d", event);
			cJSON_Delete(event_obj);
			cJSON_Delete(root_obj);
			return;
	}
	
	gl_json_add_obj(root_obj, "event", event_obj);

	char* payload = cJSON_PrintUnformatted(root_obj);

	if(!is_testing_mode())
	{
		LOG_INF("Send trigger ev: %s", payload);
		coap_send_request(COAP_METHOD_PUT, (const struct sockaddr *)&unique_local_addr,
				trigger_repo_option, payload, strlen(payload) + 1, on_send_trigger_reply);
		light_onoff();	
	}else {
		LOG_INF("Send trigger ev to testing light resource");
		coap_send_request(COAP_METHOD_PUT, (const struct sockaddr *)&multicast_local_addr,
				testing_light_option, payload, strlen(payload) + 1, NULL);
	}

	free(payload); //cJSON_FreeString

end:
	cJSON_Delete(root_obj);

	return;
}


static void send_provisioning_request(struct k_work *item)
{
	ARG_UNUSED(item);

	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
		/* decrease the polling period for higher responsiveness */
		poll_period_response_set();
	}

	LOG_INF("Send 'provisioning' request");
	coap_send_request(COAP_METHOD_GET, (const struct sockaddr *)&multicast_local_addr,
			  provisioning_option, NULL, 0u, on_provisioning_reply);
}

static void do_factory_reset(struct k_work *item)
{
	ARG_UNUSED(item);

#ifdef CONFIG_OPENTHREAD_SRP_CLIENT
	srp_utils_host_remove();
#endif
	k_sleep(K_MSEC(1000));

	ot_factoryreset();

	return;
}

static int on_send_status_reply(const struct coap_packet *response, struct coap_reply *reply,
				 const struct sockaddr *from)
{
	ARG_UNUSED(response);
	ARG_UNUSED(reply);
	ARG_UNUSED(from);

	LOG_INF("Send 'status' done.");
	return 0;
}

static void do_report_status_request(struct k_work *item)
{
	ARG_UNUSED(item);
	char *payload;

	if (!is_connected)
		return;
	if (unique_local_addr.sin6_addr.s6_addr16[0] == 0) {
		LOG_WRN("Peer address not set");
		coap_client_send_provisioning_request();
		return;
	}

	gl_sensor_sample_fetch();

	cJSON *root_obj = cJSON_CreateObject();
	gl_json_add_str(root_obj, "version", ot_get_version());
	gl_json_add_number(root_obj, "thread_version", ot_get_thread_version());
	gl_json_add_str(root_obj, "eui64", ot_get_eui64());
	gl_json_add_str(root_obj, "extaddr", ot_get_extaddr());
	gl_json_add_str(root_obj, "addr", ot_get_mleid());
	gl_json_add_number(root_obj, "rloc16", ot_get_rloc16());
	gl_json_add_str(root_obj, "sw_ver", CONFIG_SW_VERSION);
	gl_json_add_number(root_obj, "report_intervel", report_interval_second);
	gl_json_add_str(root_obj, "dev_fw_type", ot_get_device_type());
	cJSON *data_obj = cJSON_CreateObject();
	gl_json_add_number(data_obj, "temperature", gl_sensor_get_temp());
	gl_json_add_number(data_obj, "humidity", gl_sensor_get_humi());
	gl_json_add_number(data_obj, "light", gl_sensor_get_light());
	gl_json_add_number(data_obj, "press", gl_sensor_get_press());
	gl_json_add_number(data_obj, "battery_level", gl_battery_get_level());
	gl_json_add_obj(root_obj, "data", data_obj);
	payload = cJSON_PrintUnformatted(root_obj);

	LOG_INF("Send 'status' request to: %s, payload: %s", unique_local_addr_str, payload);
	coap_send_request(COAP_METHOD_PUT, (const struct sockaddr *)&unique_local_addr,
			  status_option, payload, strlen(payload) + 1, on_send_status_reply);

	free(payload); //cJSON_FreeString
	cJSON_Delete(root_obj);

	light_onoff();
}

static void toggle_minimal_sleepy_end_device(struct k_work *item)
{
	otError error;
	otLinkModeConfig mode;
	struct openthread_context *context = openthread_get_default_context();

	__ASSERT_NO_MSG(context != NULL);

	openthread_api_mutex_lock(context);
	mode = otThreadGetLinkMode(context->instance);
	mode.mRxOnWhenIdle = !mode.mRxOnWhenIdle;
	error = otThreadSetLinkMode(context->instance, mode);
	openthread_api_mutex_unlock(context);

	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set MLE link mode configuration");
	} else {
		on_mtd_mode_toggle(mode.mRxOnWhenIdle);
	}
}

static void update_device_state(void)
{
	struct otInstance *instance = openthread_get_default_instance();
	otLinkModeConfig mode = otThreadGetLinkMode(instance);
	on_mtd_mode_toggle(mode.mRxOnWhenIdle);
}

static void on_thread_state_changed(uint32_t flags, void *context)
{
	struct openthread_context *ot_context = context;

	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_context->instance)) {
		case OT_DEVICE_ROLE_CHILD:
		case OT_DEVICE_ROLE_ROUTER:
		case OT_DEVICE_ROLE_LEADER:
			k_work_submit(&on_connect_work);
			is_connected = true;
			coap_client_send_provisioning_request();
			ot_print_network_info();
#ifdef CONFIG_OPENTHREAD_SRP_CLIENT
			if (!is_srp_client_running) {
				// Set host name
				srp_utils_set_host_name(ot_get_extaddr());

				// Set host address
				char *ipaddr = ot_get_slaac_addr();
				if (strlen(ipaddr) == 0) {
					ipaddr = ot_get_mleid();
				}
				srp_utils_set_host_address(ipaddr);

				// Set service
				char instance[128] = {0};
				strcat(instance, "GL_TDB_");
				strcat(instance, ot_get_extaddr());
				srp_utils_add_service(instance, "_coap._udp", 12345);

				srp_utils_autostart(do_after_srp_srv_reg);

				is_srp_client_running = true;
			}
#endif
			break;

		case OT_DEVICE_ROLE_DISABLED:
		case OT_DEVICE_ROLE_DETACHED:
			k_work_submit(&on_disconnect_work);
			is_connected = false;
			unique_local_addr.sin6_addr.s6_addr16[0] = 0;
		default:
			break;
		}
	}

	if (flags & OT_CHANGED_JOINER_STATE) {
		int state = otJoinerGetState(ot_context->instance);
		switch (state) {
		case OT_JOINER_STATE_IDLE:
		{
			static int try_join_time = 0;
			
			try_join_time++;
			if(try_join_time > 1000)
			{	
				try_join_time = 0;
				led_off(LED2);
				if (!is_joined) {
					LOG_WRN("Join failed");
					is_joined = false;
				}
			}else{
				if(k_uptime_get_32() - join_time_start <= JOIN_COMMISSIONING_TIMEOUT){
					LOG_INF("Start joiner again... %d", try_join_time);
					start_joiner();
				}else{
					led_toggle_stop();
				}
			}
			break;
		}
		case OT_JOINER_STATE_JOINED:
			is_joined = true;
			break;
		default:
			break;
		}
	}
}

static void submit_work_if_connected(struct k_work *work)
{
	if (is_connected) {
		k_work_submit(work);
	} else {
		LOG_INF("Connection is broken");
	}
}

static otError coap_send_utils(otMessage *request_message, const otMessageInfo *message_info,
			       const void *payload, uint16_t payload_size)
{
	otError error = OT_ERROR_NO_BUFS;
	otMessage *response;

	response = otCoapNewMessage(srv_context.ot, NULL);
	if (response == NULL) {
		LOG_ERR("otCoapNewMessage failed.");
		goto end;
	}

	otCoapMessageInit(response, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_CONTENT);

	error = otCoapMessageSetToken(response, otCoapMessageGetToken(request_message),
				      otCoapMessageGetTokenLength(request_message));
	if (error != OT_ERROR_NONE) {
		LOG_ERR("otCoapMessageSetToken failed.");
		goto end;
	}

	error = otCoapMessageSetPayloadMarker(response);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("otCoapMessageSetPayloadMarker failed.");
		goto end;
	}

	error = otMessageAppend(response, payload, payload_size);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("otMessageAppend failed.");
		goto end;
	}

	error = otCoapSendResponse(srv_context.ot, response, message_info);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("otCoapSendResponse failed.");
	}
	// LOG_HEXDUMP_INF(payload, payload_size, "Sent provisioning response:");
	LOG_INF("Sent provisioning response: %d, %s", payload_size, (char *)payload);

end:
	if (error != OT_ERROR_NONE && response != NULL) {
		otMessageFree(response);
	}

	return error;
}

static void cmd_request_handler(void *context, otMessage *message,
				const otMessageInfo *message_info)
{
	char buf[1500];
	int length;
	int ret = -1;

	ARG_UNUSED(context);

	if (otCoapMessageGetType(message) != OT_COAP_TYPE_NON_CONFIRMABLE) {
		LOG_ERR("Light handler - Unexpected type of message");
		goto end;
	}

	if (otCoapMessageGetCode(message) != OT_COAP_CODE_PUT) {
		LOG_ERR("Light handler - Unexpected CoAP code");
		goto end;
	}
	length = otMessageRead(message, otMessageGetOffset(message), buf, sizeof(buf) - 1);
	buf[length] = '\0';

	LOG_INF("Received cmd request: %s", buf);

	otError error;
	otMessageInfo msg_info;
	msg_info = *message_info;

	char* resp;
	int resp_len;

	cJSON *resp_obj = cJSON_CreateObject();
	ret = srv_context.cmd_request(buf, resp_obj);

	resp = cJSON_PrintUnformatted(resp_obj);

	if (ret == CONFIG_CMD_FACTORYRESET) {

		error = coap_send_utils(message, &msg_info, resp, strlen(resp));
		if (error != OT_ERROR_NONE) {
			LOG_INF("coap_send_utils failed. error = %d", error);
			goto end;
		}
		k_sleep(K_MSEC(1000));

		free(resp);
		cJSON_Delete(resp_obj);

		k_work_submit(&factory_reset_work);

		return;

	} else if (ret == CONFIG_CMD_REBOOT) {

		error = coap_send_utils(message, &msg_info, resp, strlen(resp));
		if (error != OT_ERROR_NONE) {
			LOG_INF("coap_send_utils failed. error = %d", error);
			goto end;
		}
		k_sleep(K_MSEC(3000));

		free(resp);
		cJSON_Delete(resp_obj);

		sys_reboot(SYS_REBOOT_WARM);

		return;
	}


	error = coap_send_utils(message, &msg_info, resp, strlen(resp));
	if (error != OT_ERROR_NONE) {
		LOG_INF("coap_send_utils failed. error = %d", error);
	}

end:
	free(resp);
	cJSON_Delete(resp_obj);
	return;
}

static void coap_default_handler(void *context, otMessage *message,
				 const otMessageInfo *message_info)
{
	ARG_UNUSED(context);
	ARG_UNUSED(message);
	ARG_UNUSED(message_info);

	LOG_INF("Received CoAP message that does not match any request "
		"or resource");
}

static int cmd_request(const char *json_str, cJSON* resp_obj)
{
	int ret = ERROR_CODE_NONE;
	const char *cmd = NULL;
	int cmd_id;
	const char *obj = NULL;
	cJSON *root_obj = NULL;

	root_obj = cJSON_Parse(json_str);
	if (root_obj == NULL) {
		LOG_ERR("cJSON Parse failure");
		ret = ERROR_CODE_INVALID_PARAMETER;
		return ERROR_CODE_INVALID_PARAMETER;;
	}

	cmd = gl_json_get_string(root_obj, "cmd");
	cmd_id = get_cmd_id(cmd);
	switch (cmd_id) {
	case CONFIG_CMD_ON_OFF: {
		obj = gl_json_get_string(root_obj, "obj");
		if (!check_obj_valid(obj)) {
			LOG_ERR("obj error");
			ret = ERROR_CODE_INVALID_PARAMETER;
			goto out;
		}

		for(int i = 0; i < ARRAY_SIZE(g_led_obj); i++)
		{
			if (0 == strcmp(g_led_obj[i].obj, obj))
			{
				int val = gl_json_get_int(root_obj, "val");
				int delay_s = gl_json_get_int(root_obj, "delay");
				if((delay_s >= 0))
				{
					if(0 != on_off_led_strip_with_delay(g_led_obj[i].id, 0, delay_s * 1000))
					{
						LOG_ERR("on_off_led_strip_with_delay ERROR");
						ret = ERROR_CODE_UNKNOW;
						goto out;
					}
				}

				if(0 != on_off_led_strip(g_led_obj[i].id, val))
				{
					LOG_ERR("on_off_led_strip ERROR");
					ret = ERROR_CODE_UNKNOW;
					goto out;
				}
				
			}
		}

	} break;
	case CONFIG_CMD_CHANGE_COLOR: {
		obj = gl_json_get_string(root_obj, "obj");
		if (!check_obj_valid(obj)) {
			LOG_ERR("obj error");
			ret = ERROR_CODE_INVALID_PARAMETER;
			goto out;
		}

		for(int i = 0; i < ARRAY_SIZE(g_led_obj); i++)
		{
			if (0 == strcmp(g_led_obj[i].obj, obj))
			{
				struct led_rgb color;
				color.r = gl_json_get_int(root_obj, "r");
				color.g = gl_json_get_int(root_obj, "g");
				color.b = gl_json_get_int(root_obj, "b");

				if(0 != update_led_strip_rgb(g_led_obj[i].id, &color))
				{
					LOG_ERR("update_led_strip_rgb ERROR");
					ret = ERROR_CODE_UNKNOW;
					goto out;
				}
			}
		}

	}break;
	case CONFIG_CMD_SET_GPIO: {
		obj = gl_json_get_string(root_obj, "obj");
		int val = gl_json_get_boolean(root_obj, "val");

		if(0 != gl_set_gpio_status_by_name(obj, val))
		{
			LOG_ERR("gl_set_gpio_status_by_name ERROR");
			ret = ERROR_CODE_UNKNOW;
		}
	}break;
	case CONFIG_CMD_GET_LED_STATUS:{
		cJSON *array_obj = cJSON_CreateArray();
		int on_off;
		struct led_rgb color;
		for(size_t i = 1; i < 3; i++)
		{
			cJSON *item_obj = cJSON_CreateObject();
			if(0 != get_led_strip_status(LED_STRIP_NODE_1, &on_off, &color))
			{
				LOG_ERR("get_led_strip_status ERROR");
				ret = ERROR_CODE_UNKNOW;
				goto out;
			}
			cJSON_AddStringToObjectCS(item_obj, "obj", "led_left");
			cJSON_AddNumberToObjectCS(item_obj, "on_off", on_off);
			cJSON_AddNumberToObjectCS(item_obj, "r", color.r);
			cJSON_AddNumberToObjectCS(item_obj, "g", color.g);
			cJSON_AddNumberToObjectCS(item_obj, "b", color.b);
			cJSON_AddItemToArray(array_obj, item_obj);
		}
		cJSON_AddItemToObjectCS(resp_obj, "led_strip_status", array_obj);
	}break;
	case CONFIG_CMD_GET_GPIO_STATUS: {
		cJSON *array_obj = cJSON_CreateArray();
		for(size_t i = 0; i < 4; i++)
		{
			cJSON *item_obj = cJSON_CreateObject();
			cJSON_AddStringToObjectCS(item_obj, "obj", gl_get_gpio_name(i));
			cJSON_AddNumberToObjectCS(item_obj, "val", gl_get_gpio_status(i));
			cJSON_AddItemToArray(array_obj, item_obj);
		}
		cJSON_AddItemToObjectCS(resp_obj, "gpio_status", array_obj);
	}break;
	case CONFIG_CMD_SET_REPORT_INTERVAL: {
		obj = gl_json_get_string(root_obj, "obj");
		int val = gl_json_get_int(root_obj, "val");
		report_interval_second = val;

		if(val > 0){
			k_timer_stop(&report_timer);
			k_timer_start(&report_timer, K_MSEC(3000), K_MSEC(report_interval_second * 1000));
		}
	}break;
	case CONFIG_CMD_SET_OT_MODE: {
		obj = gl_json_get_string(root_obj, "obj");
		char *mode_str = gl_json_get_string(root_obj, "val");
		otLinkModeConfig mode = {
			.mRxOnWhenIdle = strchr(mode_str, 'r') ? true : false,
			.mDeviceType = strchr(mode_str, 'd') ? true : false,
			.mNetworkData = strchr(mode_str, 'n') ? true : false
		};
		
		struct openthread_context *context = openthread_get_default_context();
		otError error;
		__ASSERT_NO_MSG(context != NULL);

		openthread_api_mutex_lock(context);
		error = otThreadSetLinkMode(context->instance, mode);
		openthread_api_mutex_unlock(context);
		
		if(error == OT_ERROR_NONE){
			ret = ERROR_CODE_NONE;
		}else if(error == OT_ERROR_INVALID_ARGS){
			ret = ERROR_CODE_INVALID_PARAMETER;
		}else{
			ret = ERROR_CODE_UNKNOW;
		}
	}break;
	case CONFIG_CMD_UPGRADE:
	case CONFIG_CMD_FACTORYRESET:
	case CONFIG_CMD_REBOOT:
		cJSON_AddNumberToObjectCS(resp_obj, "err_code", ERROR_CODE_NONE);
		cJSON_Delete(root_obj);
		return cmd_id;
	default:
		break;
	}

	LOG_INF("cmd_id = %d, cmd = %s, obj = %s", cmd_id, cmd, obj);

out:
	cJSON_AddNumberToObjectCS(resp_obj, "err_code", ret);
end:
	cJSON_Delete(root_obj);
	return ERROR_CODE_NONE;
}

static void on_report_timer_expiry(struct k_timer *timer_id)
{
	ARG_UNUSED(timer_id);

	if (!is_connected) {
		LOG_WRN("Network disconnect.");
		return;
	}
	if (unique_local_addr.sin6_addr.s6_addr16[0] == 0) {
		LOG_WRN("Peer address not set");
		coap_client_send_provisioning_request();
		return;
	}
	coap_client_send_status();
}

static void on_report_timer_stop(struct k_timer *timer_id)
{
	ARG_UNUSED(timer_id);
	LOG_INF("%d,%s", __LINE__, __FUNCTION__);
}

void ot_link_mode_init()
{
	struct openthread_context *context = openthread_get_default_context();
	otError error;
	__ASSERT_NO_MSG(context != NULL);

	openthread_api_mutex_lock(context);
	otLinkModeConfig mode = {
#ifdef	CONFIG_OPENTHREAD_MTD
		.mRxOnWhenIdle = true,
		.mDeviceType = false,
		.mNetworkData = false
#else
		.mRxOnWhenIdle = true,
		.mDeviceType = true,
		.mNetworkData = true
#endif
	};
	error = otThreadSetLinkMode(context->instance, mode);
	openthread_api_mutex_unlock(context);
	
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set link mode, error:%d",error);
	} else {
		// on_mtd_mode_toggle(mode.mRxOnWhenIdle);
		LOG_INF("Set %s link mode, error:%d", IS_ENABLED(CONFIG_OPENTHREAD_MTD) ? "MTD" : "FTD", error);
	}
}

void coap_client_utils_init(ot_connection_cb_t on_connect, ot_disconnection_cb_t on_disconnect,
			    mtd_mode_toggle_cb_t on_toggle)
{
	on_mtd_mode_toggle = on_toggle;

	coap_init(AF_INET6, NULL);
	ot_link_mode_init();

	k_timer_init(&report_timer, on_report_timer_expiry, on_report_timer_stop);
	k_work_init(&factory_reset_work, do_factory_reset);


	k_work_init(&on_connect_work, on_connect);
	k_work_init(&on_disconnect_work, on_disconnect);
	k_work_init(&provisioning_work, send_provisioning_request);
	k_work_init(&report_status_work, do_report_status_request);

	openthread_set_state_changed_cb(on_thread_state_changed);
	
	struct openthread_context *context = openthread_get_default_context();

	if (otDatasetIsCommissioned(context->instance)) {
		openthread_start(context);
	}

	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
		k_work_init(&toggle_MTD_SED_work, toggle_minimal_sleepy_end_device);
		// update_device_state();
	}

	cmd_resource.mContext = srv_context.ot;
	cmd_resource.mHandler = cmd_request_handler;

	testing_light_resource.mContext = srv_context.ot;
	testing_light_resource.mHandler = testing_light_request_handler;

	srv_context.cmd_request = cmd_request;
	srv_context.ot = context->instance;
	if (!srv_context.ot) {
		LOG_ERR("There is no valid OpenThread instance");
	}

	otCoapSetDefaultHandler(srv_context.ot, coap_default_handler, NULL);
	otCoapAddResource(srv_context.ot, &cmd_resource);
	otCoapAddResource(srv_context.ot, &testing_light_resource);

	if (otCoapStart(srv_context.ot, COAP_PORT) != OT_ERROR_NONE) {
		LOG_ERR("Failed to start OT CoAP.");
	}

	otPlatRadioSetTransmitPower(context->instance, 8);

	ot_print_network_info();
}


void coap_client_send_provisioning_request(void)
{
	if(is_testing_mode())
	{
		return;
	}
	submit_work_if_connected(&provisioning_work);
}

void coap_client_toggle_minimal_sleepy_end_device(void)
{
	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
		k_work_submit(&toggle_MTD_SED_work);
	}
}

void coap_client_send_status(void)
{
	submit_work_if_connected(&report_status_work);
}

void do_after_srp_srv_reg(void)
{
#ifdef CONFIG_MCUMGR_SMP_UDP
	smp_start();
#endif

	k_timer_start(&report_timer, K_MSEC(3000), K_MSEC(report_interval_second * 1000));

	return;
}