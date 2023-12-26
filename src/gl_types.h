/*****************************************************************************
 * @file  gl_type.h
 * @brief Define various types.
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

#ifndef _GL_TYPES_H_
#define _GL_TYPES_H_

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

enum { 
    ERROR_CODE_UNKNOW = -3,
    ERROR_CODE_NO_BUFS = -2, 
    ERROR_CODE_INVALID_PARAMETER = -1, 
    ERROR_CODE_NONE = 0 
};

enum { 
    CONFIG_CMD_ON_OFF = 0, 
    CONFIG_CMD_UPGRADE, 
    CONFIG_CMD_FACTORYRESET, 
    CONFIG_CMD_REBOOT,
    CONFIG_CMD_CHANGE_COLOR,
    CONFIG_CMD_SET_GPIO,
    CONFIG_CMD_GET_LED_STATUS,
    CONFIG_CMD_GET_GPIO_STATUS,
    CONFIG_CMD_SET_REPORT_INTERVAL,
    CONFIG_CMD_SET_OT_MODE
};

enum { 
    CONFIG_OBJ_LED_STRIP_NODE_ALL = 0,
    CONFIG_OBJ_LED_STRIP_NODE_LEFT = 1,
    CONFIG_OBJ_LED_STRIP_NODE_RIGHT = 2   
};



enum { DEVICE_INITIAL = 0, DEVICE_CONNECTING = 1, DEVICE_CONNECTED = 2, DEVICE_DISCONNECTED = 3 };

#endif /* _GL_TYPES_H_ */