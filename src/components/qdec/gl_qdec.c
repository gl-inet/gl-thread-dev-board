/*****************************************************************************
 * @file  gl_qdec.c
 * @brief Provide qdec device related functions
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
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "gl_qdec.h"

LOG_MODULE_REGISTER(qdec, CONFIG_GL_THREAD_DEV_BOARD_LOG_LEVEL);

const struct device *qdec = DEVICE_DT_GET(DT_ALIAS(qdec0)); //旋钮

void gl_qdec_init(sensor_trigger_handler_t handler)
{
	if (!device_is_ready(qdec)) {
		LOG_ERR("Qdec device is not ready\n");
		return;
	}

	LOG_INF("Init QDEC success!\n");

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ROTATION,
	};
	sensor_trigger_set(qdec, &trig, handler);
	return;
}
