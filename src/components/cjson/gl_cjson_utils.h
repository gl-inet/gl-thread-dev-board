/*****************************************************************************
 * @file  gl_cjson_utils.h
 * @brief The header file of gl_cjson_utils.c
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

#ifndef _GL_CJSON_UTILS_H_
#define _GL_CJSON_UTILS_H_

#include <stdbool.h>
#include <cJSON.h>
#include <cJSON_os.h>

int gl_json_add_obj(cJSON *obj, const char *key, cJSON *val);
int gl_json_add_str(cJSON *obj, const char *key, const char *val);
int gl_json_add_number(cJSON *obj, const char *key, double val);
int gl_json_add_boolean(cJSON *obj, const char *key, int val);

bool gl_json_get_boolean(cJSON *obj, const char *key);
int gl_json_get_int(cJSON *obj, const char *key);
char *gl_json_get_string(cJSON *obj, const char *key);

#endif /* _GL_CJSON_UTILS_H_ */