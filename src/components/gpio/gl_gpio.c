/*****************************************************************************
 * @file  gl_gpio.c
 * @brief Provide gpio related functions
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
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "gl_gpio.h"

#define GPIOS_NODE DT_PATH(glios)
#define GPIO_SPEC_AND_COMMA(glios) GPIO_DT_SPEC_GET(glios, gpios),

static const struct gpio_dt_spec gl_ios[] = {
#if DT_NODE_EXISTS(GPIOS_NODE)
	DT_FOREACH_CHILD(GPIOS_NODE, GPIO_SPEC_AND_COMMA)
#endif
};

LOG_MODULE_REGISTER(gl_gpio, CONFIG_GL_THREAD_DEV_BOARD_LOG_LEVEL);

struct gpio_node {
	gl_gpio_node_e      id;
	char                name[16];
    gl_gpio_status_e    status;
} gl_gpio_obj[] = {
	{ GPIO_015, "0.15", GPIO_LOW },
	{ GPIO_016, "0.16", GPIO_LOW },
	{ GPIO_017, "0.17", GPIO_LOW },
	{ GPIO_020, "0.20", GPIO_LOW },
};

static gl_gpio_node_e _get_gpio_id_by_name(char* name)
{
    for(size_t i = 0; i < ARRAY_SIZE(gl_gpio_obj); i++)
    {
        if(0 == strcmp(gl_gpio_obj[i].name, name))
        {
            return gl_gpio_obj[i].id;
        }
    }

    return GPIO_NULL;
}

int gl_gpio_init(void)
{
	int err;
    size_t i;

    if(ARRAY_SIZE(gl_gpio_obj) != ARRAY_SIZE(gl_ios))
    {
        LOG_ERR("GPIO number error!");
        return -1;
    }

	for (i = 0; i < ARRAY_SIZE(gl_ios); i++) {
		err = gpio_pin_configure_dt(&gl_ios[i], GPIO_OUTPUT);
		if (err) {
			LOG_ERR("Cannot configure OUTPUT gpio");
			return err;
		}

        gl_gpio_obj[i].status = GPIO_LOW;

        err = gl_set_gpio_status_by_id(gl_gpio_obj[i].id, gl_gpio_obj[i].status);
        if (err) {
            LOG_ERR("Cannot init OUTPUT gpio");
            return err;
        }
	}

	return 0;
}

gl_gpio_status_e gl_get_gpio_status(gl_gpio_node_e node)
{
    return gl_gpio_obj[node].status;
}

char* gl_get_gpio_name(gl_gpio_node_e node)
{
    return gl_gpio_obj[node].name;
}

int gl_set_gpio_status_by_id(gl_gpio_node_e node_id, gl_gpio_status_e status)
{
    if(node_id >= ARRAY_SIZE(gl_gpio_obj))
    {
        LOG_ERR("GPIO node don't exist");
        return -1;
    }

    gl_gpio_obj[node_id].status = status;

    int err = gpio_pin_set_dt(&gl_ios[node_id], status);
    if (err) {
        LOG_ERR("Cannot write OUTPUT gpio");
        return err;
    }
    
    return 0;
}

int gl_set_gpio_status_by_name(char* node_name, gl_gpio_status_e status)
{
    if(node_name == NULL)
    {
        LOG_ERR("GPIO node don't exist");
        return -1;
    }

    gl_gpio_node_e id = _get_gpio_id_by_name(node_name);
    if(id == GPIO_NULL)
    {
        LOG_ERR("GPIO node don't exist");
        return -1;
    }

    gl_gpio_obj[id].status = status;

    int err = gpio_pin_set_dt(&gl_ios[id], status);
    if (err) {
        LOG_ERR("Cannot write OUTPUT gpio");
        return err;
    }
    
    return 0;
}