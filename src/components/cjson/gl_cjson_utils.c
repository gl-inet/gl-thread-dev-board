/*****************************************************************************
 * @file  gl_cjson_utils.c
 * @brief Provide cjson function.
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
#include <zephyr/logging/log.h>

#include "gl_cjson_utils.h"
#include "gl_types.h"

LOG_MODULE_REGISTER(gl_cjson_utils, CONFIG_GL_CJSON_UTILS_LOG_LEVEL);

int gl_json_add_obj(cJSON *obj, const char *key, cJSON *val)
{
	cJSON_AddItemToObject(obj, key, val);

	return 0;
}

int gl_json_add_str(cJSON *obj, const char *key, const char *val)
{
	cJSON *json_str;

	json_str = cJSON_CreateString(val);
	if (json_str == NULL) {
		return ERROR_CODE_NO_BUFS;
	}

	return cJSON_AddItemToObject(obj, key, json_str);
}

int gl_json_add_number(cJSON *obj, const char *key, double val)
{
	cJSON *json_num;

	json_num = cJSON_CreateNumber(val);
	if (json_num == NULL) {
		return ERROR_CODE_NO_BUFS;
	}

	return cJSON_AddItemToObject(obj, key, json_num);
}

int gl_json_add_boolean(cJSON *obj, const char *key, int val)
{
	cJSON *json_num;

	json_num = cJSON_CreateBool(val);
	if (json_num == NULL) {
		return ERROR_CODE_NO_BUFS;
	}

	return cJSON_AddItemToObject(obj, key, json_num);
}

bool gl_json_get_boolean(cJSON *obj, const char *key)
{
	if (!cJSON_HasObjectItem(obj, key)) {
		LOG_ERR("Invalid parameter.");
		return false;
	}

	return cJSON_GetObjectItem(obj, key)->valueint ? true : false;
}

int gl_json_get_int(cJSON *obj, const char *key)
{
	if (!cJSON_HasObjectItem(obj, key)) {
		LOG_ERR("Invalid parameter.");
		return -EINVAL;
	}

	return cJSON_GetObjectItem(obj, key)->valueint;
}

char *gl_json_get_string(cJSON *obj, const char *key)
{
	if (!cJSON_HasObjectItem(obj, key)) {
		LOG_ERR("Invalid parameter.");
		return NULL;
	}

	return cJSON_GetObjectItem(obj, key)->valuestring;
}