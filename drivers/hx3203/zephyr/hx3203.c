/*****************************************************************************
 * @file  hx3203.c
 * @brief The driver of hx3203.
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

#define DT_DRV_COMPAT tianyihexin_hx3203

#include "hx3203.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/types.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(HX3203, CONFIG_SENSOR_LOG_LEVEL);

int hx3203_read(const struct device *dev, uint8_t reg, uint8_t *out)
{
	const struct hx3203_config *config = dev->config;
	int ret = 0;

	ret = i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), out, 1);

	return ret;
}

int hx3203_write(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct hx3203_config *config = dev->config;

	struct i2c_msg msg;
	int ret;
	uint8_t buff[3];

	sys_put_le16(value, &buff[1]);

	buff[0] = reg;

	msg.buf = buff;
	msg.flags = 0;
	msg.len = sizeof(buff);

	ret = i2c_write_dt(&config->i2c, buff, 3);

	if (ret < 0) {
		LOG_ERR("write block failed");
		return ret;
	}

	return 0;
}

uint8_t hx3203_get_device_id(const struct device *dev)
{
	uint8_t id;

	if (hx3203_read(dev, HX3203_REG_DEVICE_ID, &id) != 0) {
		LOG_ERR("hx3203_read failed!");
		return -EINVAL;
	}

	return id;
}

void hx3203_enable(const struct device *dev)
{
	// hx3203_write(dev, 0x7a, 0x04);
	hx3203_write(dev, 0x01, 0x74);
	// hx3203_write(dev, 0x02, 0x0e); // int enable
}

void hx3203_disable(const struct device *dev)
{
	hx3203_write(dev, 0x0c, 0x02);
}

static int hx3203_init(const struct device *dev)
{
	const struct hx3203_config *config = dev->config;
	struct hx3203_data *data = dev->data;
	uint8_t id = 0;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}
	k_sleep(K_MSEC(50));

	id = hx3203_get_device_id(dev);
	if (id != HX3203_DEFAULT_DEVICE_ID) {
		LOG_ERR("failed!");
		return -EIO;
	}

	hx3203_write(dev, 0x01, 0x70);
	hx3203_write(dev, 0x02, 0x06);
	hx3203_write(dev, 0x0c, 0x22);
	hx3203_write(dev, 0x16, 0x36);
	hx3203_write(dev, 0x0b, 0x00);

	// LOG_DBG("hx3203_init done. i2c_name = %s, i2c_address = 0x%x, id = 0x%x", config->i2c_name,
	//        config->i2c_address, id);

	hx3203_enable(dev);
	hx3203_write(dev, 0x0c, 0x22);

	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);
	k_sem_give(&data->sem);

	return 0;
}

static int hx3203_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct hx3203_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT) {
		data->light = 0;
		int16_t temp_data = 0;
		uint8_t databuf[3];
		uint32_t ch0_data = 0;
		uint32_t ch1_data = 0;
		uint16_t als_max = 0;
		ret = hx3203_read(dev, 0x80, &databuf[0]);
		if (ret < 0) {
			LOG_ERR("Could not fetch ambient light");
		}
		ret = hx3203_read(dev, 0x81, &databuf[1]);
		if (ret < 0) {
			LOG_ERR("Could not fetch ambient light");
		}
		als_max = (((databuf[1] & 0x01) << 4) | ((databuf[0] >> 4)));

		ret = hx3203_read(dev, HX3203_REG_CH0_DATA_15_8, &databuf[0]);
		if (ret < 0) {
			LOG_ERR("Could not fetch ambient light");
		}
		ret = hx3203_read(dev, HX3203_REG_CH0_DATA_7_4, &databuf[1]);
		if (ret < 0) {
			LOG_ERR("Could not fetch ambient light");
		}
		ret = hx3203_read(dev, HX3203_REG_CH0_DATA_17_16_AND_3_0, &databuf[2]);
		if (ret < 0) {
			LOG_ERR("Could not fetch ambient light");
		}
		ch0_data = ((databuf[0] << 8) | ((databuf[1] & 0x0F) << 4) | (databuf[2] & 0x0F) | ((databuf[2] & 0x30) << 16));

		ret = hx3203_read(dev, 0x08, &databuf[0]);
		if (ret < 0) {
			LOG_ERR("Could not fetch ambient light");
		}
		ret = hx3203_read(dev, 0x0d, &databuf[1]);
		if (ret < 0) {
			LOG_ERR("Could not fetch ambient light");
		}
		ret = hx3203_read(dev, 0x0e, &databuf[2]);
		if (ret < 0) {
			LOG_ERR("Could not fetch ambient light");
		}
		ch1_data = ((databuf[0] << 3) | ((databuf[1] & 0x3F) << 11) | (databuf[2] & 0x07));

		temp_data = ch0_data - als_max - (ch1_data * 145 / 100);
		if ((ch0_data > 16380) || (ch1_data > 16380)) {
			temp_data = 16384;
		}
		if (temp_data < 0) {
			temp_data = 0;
		}

		data->light = temp_data;
	}

	k_sem_give(&data->sem);

	return ret;
}

static int hx3203_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct hx3203_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = data->light;
		val->val2 = 0;
		break;
	default:
		ret = -ENOTSUP;
	}

	k_sem_give(&data->sem);

	return ret;
}

static const struct sensor_driver_api hx3203_driver_api = {
	.sample_fetch = hx3203_sample_fetch,
	.channel_get = hx3203_channel_get,
};

#define HX3203_CONFIG(inst)                                                                        \
	{                                                                                          \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	}

#define HX3203_DEFINE(inst)                                                                        \
	static struct hx3203_data hx3203_data_##inst;                                              \
	static struct hx3203_config hx3203_config_##inst = HX3203_CONFIG(inst);                    \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, hx3203_init, NULL, &hx3203_data_##inst,                 \
				     &hx3203_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &hx3203_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HX3203_DEFINE)