/*****************************************************************************
 * @file   main.c
 * @brief  The start of application.
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
#include <stdlib.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>

#include <openthread/thread.h>
#include <openthread/instance.h>

#ifdef CONFIG_BOARD_NRF52840DONGLE_NRF52840
#include <drivers/uart.h>
#include <usb/usb_device.h>
#endif

#ifdef CONFIG_BOOTLOADER_MCUBOOT
#include <zephyr/dfu/mcuboot.h>
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
#include "stat_mgmt/stat_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_SMP_UDP
#include "gl_smp_udp.h"
#endif
#ifdef CONFIG_LED_STRIP
#include "gl_led_strip.h"
#endif

#include "gl_cjson_utils.h"
#include "gl_qdec.h"
#include "gl_gpio.h"
#include "gl_led.h"
#include "gl_coap.h"
#include "gl_ot_api.h"
#include "gl_sensor.h"
#include "gl_button_logic.h"
#include "gl_types.h"
#include "gl_battery.h"

LOG_MODULE_REGISTER(main, CONFIG_GL_THREAD_DEV_BOARD_LOG_LEVEL);


int joiner_state;


static void on_ot_connect(struct k_work *item)
{
	ARG_UNUSED(item);

	if (joiner_state != DEVICE_CONNECTED) {
		LOG_WRN("device connected.");
		joiner_state = DEVICE_CONNECTED;
		led_toggle_stop();
	}

	dk_set_led_on(LED2);
}

static void on_ot_disconnect(struct k_work *item)
{
	ARG_UNUSED(item);

	if (joiner_state != DEVICE_DISCONNECTED) {
		LOG_WRN("device disconnected.");
		joiner_state = DEVICE_DISCONNECTED;
		led_toggle_stop();
		led_toggle_start(1000);
	}

	dk_set_led_off(LED2);
}

static void on_mtd_mode_toggle(uint32_t med)
{
#if IS_ENABLED(CONFIG_PM_DEVICE)
	const struct device *cons = device_get_binding(CONSOLE_LABEL);

	if (med) {
		pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);
	} else {
		pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	}
#endif
	dk_set_led(LED2, med);
}

static struct k_work qdec_work;
#define DEF_ROTATION	(24)
static double rotation = 0;
static void qdec_trigger(struct k_work *item)
{
	ARG_UNUSED(item);

	if(rotation == 0)
	{
		return;
	}

	static double tmp_rotation = 0;
	tmp_rotation = rotation;
	rotation = 0;
	send_trigger_event_request(QDEC_ROTATE_TRIGGER, "qdec_0", (void*)&tmp_rotation);

}
void encoder_callback(const struct device *dev, const struct sensor_trigger *trigger)
{
	struct sensor_value val;

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);

	printk("current %d.%d\r\n", val.val1, val.val2);

	if(val.val1 > 0)
	{
		if(rotation < 0)
		{
			rotation = 0;
		}
	}else{
		if(rotation > 0)
		{
			rotation = 0;
		}
	}
	rotation += val.val1;

	if(abs(rotation) >= DEF_ROTATION)
	{
		k_work_submit(&qdec_work);
	}
}

void print_version(void)
{
	printk("{\"Board\":\"%s\"}\n", CONFIG_BOARD);
	printk("{\"SW_Version\":\"%s\"}\n", CONFIG_SW_VERSION);
	printk("OpenThread Stack: %s\n", otGetVersionString());
	printk("OpenThread Mode: %s\n", ot_get_mode());
}

void simulation_click(uint32_t button)
{
	on_button_changed(0xFFFF, button);
	on_button_changed(0xFFFF, button);
}

void auto_join_commissioning_start()
{
	simulation_click(DK_BTN2_MSK);
}

void main(void)
{
	print_version();

	int ret;

#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
	os_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
	img_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
	stat_mgmt_register_group();
#endif

	ret = gl_gpio_init();
	if (ret) {
		LOG_ERR("Could not initialize gpios, err code: %d", ret);
	}

	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("Could not initialize leds, err code: %d", ret);
	}
	light_off();

	ret = dk_buttons_init(on_button_changed);
	if (ret) {
		LOG_ERR("Cannot init buttons (error: %d)", ret);
	}

	ret = battery_measure_enable(true);
	if (ret) {
		LOG_ERR("Could not initialize battery measurement, err code: %d", ret);
	}

#ifdef CONFIG_BOOTLOADER_MCUBOOT
	/* Check if the image is run in the REVERT mode and eventually
	 * confirm it to prevent reverting on the next boot.
	 */
	if (mcuboot_swap_type() == BOOT_SWAP_TYPE_REVERT) {
		if (boot_write_img_confirmed()) {
			LOG_ERR("Confirming firmware image failed, it will be reverted on the next boot.");
		} else {
			LOG_INF("New device firmware image confirmed.");
		}
	}
#endif

	joiner_state = DEVICE_INITIAL;

	gl_sensor_init();

	coap_client_utils_init(on_ot_connect, on_ot_disconnect, on_mtd_mode_toggle);
	// auto_commissioning_timer_init();
	auto_join_commissioning_start();

#ifdef CONFIG_SENSOR_VALUE_AUTO_PRINT
	debug_sensor_data();
#endif
	gl_led_strip_init();

	k_work_init(&qdec_work, qdec_trigger);
	gl_qdec_init(encoder_callback);

	return ;
}


