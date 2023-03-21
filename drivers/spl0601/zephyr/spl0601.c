/*****************************************************************************
 * @file  spl0601.c
 * @brief The driver of spl0601.
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

#define DT_DRV_COMPAT goertek_spl0601

#include "spl0601.h"

#include <zephyr/logging/log.h>
#include <zephyr/types.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

LOG_MODULE_REGISTER(SPL0601, CONFIG_SENSOR_LOG_LEVEL);

#define SPL0601_

uint8_t spl0601_read(const struct device *dev, uint8_t reg)
{
	uint8_t reg_value;
	int ret = 0;
	const struct spl0601_config *config = dev->config;

	ret = i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), &reg_value, 1);
	if (ret != 0) {
		LOG_ERR("spl0601_read error!");
	}

	return reg_value;
}

int spl0601_read_byte(const struct device *dev, uint8_t reg, uint8_t *value, uint8_t size)
{
	int ret = 0;
	const struct spl0601_config *config = dev->config;

	ret = i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), value, size);
	if (ret != 0) {
		LOG_ERR("spl0601_read error!");
	}

	return ret;
}

int spl0601_write(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct spl0601_config *config = dev->config;
	struct i2c_msg msg;
	int ret;
	uint8_t buff[2];

	buff[0] = reg;
	buff[1] = value;

	msg.buf = buff;
	msg.flags = 0;
	msg.len = sizeof(buff);

	ret = i2c_write_dt(&config->i2c, buff, 2);
	if (ret < 0) {
		LOG_ERR("write block failed");
		return ret;
	}
	return 0;
}

uint8_t spl0601_get_device_id(const struct device *dev)
{
	return spl0601_read(dev, SPL0601_REG_ID);
}

uint16_t spl0601_get_measurement_time(spl0601_ovsample_e oversample)
{
	uint16_t m_t = 0;

	switch (oversample) {
	case OV_SINGLE:
		m_t = SPL0601_0_OVERSAMPLE_MEASUREMENT_T;
		break;
	case OV_2:
		m_t = SPL0601_2_OVERSAMPLE_MEASUREMENT_T;
		break;
	case OV_4:
		m_t = SPL0601_4_OVERSAMPLE_MEASUREMENT_T;
		break;
	case OV_8:
		m_t = SPL0601_8_OVERSAMPLE_MEASUREMENT_T;
		break;
	case OV_16:
		m_t = SPL0601_16_OVERSAMPLE_MEASUREMENT_T;
		break;
	case OV_32:
		m_t = SPL0601_32_OVERSAMPLE_MEASUREMENT_T;
		break;
	case OV_64:
		m_t = SPL0601_64_OVERSAMPLE_MEASUREMENT_T;
		break;
	case OV_128:
		m_t = SPL0601_128_OVERSAMPLE_MEASUREMENT_T;
		break;
	default:
		LOG_ERR("Don't support %d oversample!\n", oversample);
		break;
	}

	// Make sure spl0601 got the data
	m_t = m_t + (m_t / 2);

	return m_t;
}

static void spl0601_get_calib_param(const struct device *dev)
{
	struct spl0601_data *data = dev->data;

	uint8_t h;
	uint8_t m;
	uint8_t l;

	h = spl0601_read(dev, 0x10);
	l = spl0601_read(dev, 0x11);
	data->c0 = (int16_t)h << 4 | l >> 4;
	data->c0 = (data->c0 & 0x0800) ? (0xF000 | data->c0) : data->c0;

	h = spl0601_read(dev, 0x11);
	l = spl0601_read(dev, 0x12);
	data->c1 = (int16_t)(h & 0x0F) << 8 | l;
	data->c1 = (data->c1 & 0x0800) ? (0xF000 | data->c1) : data->c1;

	h = spl0601_read(dev, 0x13);
	m = spl0601_read(dev, 0x14);
	l = spl0601_read(dev, 0x15);
	data->c00 = (uint32_t)h << 12 | (int32_t)m << 4 | (int32_t)l >> 4;
	data->c00 = (data->c00 & 0x080000) ? (0xFFF00000 | data->c00) : data->c00;

	h = spl0601_read(dev, 0x15);
	m = spl0601_read(dev, 0x16);
	l = spl0601_read(dev, 0x17);
	data->c10 = (int32_t)(h & 0x0F) << 16 | (int32_t)m << 8 | l;
	data->c10 = (data->c10 & 0x080000) ? (0xFFF00000 | data->c10) : data->c10;

	h = spl0601_read(dev, 0x18);
	l = spl0601_read(dev, 0x19);
	data->c01 = (int16_t)h << 8 | l;

	h = spl0601_read(dev, 0x1A);
	l = spl0601_read(dev, 0x1B);
	data->c11 = (int16_t)h << 8 | l;

	h = spl0601_read(dev, 0x1C);
	l = spl0601_read(dev, 0x1D);
	data->c20 = (int16_t)h << 8 | l;

	h = spl0601_read(dev, 0x1E);
	l = spl0601_read(dev, 0x1F);
	data->c21 = (int16_t)h << 8 | l;

	h = spl0601_read(dev, 0x20);
	l = spl0601_read(dev, 0x21);
	data->c30 = (int16_t)h << 8 | l;
}

void spl0601_rateset(const struct device *dev, uint8_t iSensor, uint8_t u8SmplRate,
		     uint8_t u8OverSmpl)
{
	struct spl0601_data *data = dev->data;

	uint8_t reg = 0;
	int32_t i32kPkT = 0;

	switch (u8SmplRate) {
	case 2:
		reg |= (1 << 4);
		break;
	case 4:
		reg |= (2 << 4);
		break;
	case 8:
		reg |= (3 << 4);
		break;
	case 16:
		reg |= (4 << 4);
		break;
	case 32:
		reg |= (5 << 4);
		break;
	case 64:
		reg |= (6 << 4);
		break;
	case 128:
		reg |= (7 << 4);
		break;
	case 1:
	default:
		break;
	}
	switch (u8OverSmpl) {
	case 2:
		reg |= 1;
		i32kPkT = 1572864;
		break;
	case 4:
		reg |= 2;
		i32kPkT = 3670016;
		break;
	case 8:
		reg |= 3;
		i32kPkT = 7864320;
		break;
	case 16:
		i32kPkT = 253952;
		reg |= 4;
		break;
	case 32:
		i32kPkT = 516096;
		reg |= 5;
		break;
	case 64:
		i32kPkT = 1040384;
		reg |= 6;
		break;
	case 128:
		i32kPkT = 2088960;
		reg |= 7;
		break;
	case 1:
	default:
		i32kPkT = 524288;
		break;
	}

	if (iSensor == SPL0601_PRESSURE_SENSOR) {
		data->i32kP = i32kPkT;
		spl0601_write(dev, 0x06, reg);
		if (u8OverSmpl > 8) {
			reg = spl0601_read(dev, 0x09);
			spl0601_write(dev, 0x09, reg | 0x04);
		} else {
			reg = spl0601_read(dev, 0x09);
			spl0601_write(dev, 0x09, reg & (~0x04));
		}
	}
	if (iSensor == SPL0601_TEMPERATURE_SENSOR) {
		data->i32kT = i32kPkT;
		spl0601_write(dev, 0x07, reg | 0x80); //Using mems temperature
		if (u8OverSmpl > 8) {
			reg = spl0601_read(dev, 0x09);
			spl0601_write(dev, 0x09, reg | 0x08);
		} else {
			reg = spl0601_read(dev, 0x09);
			spl0601_write(dev, 0x09, reg & (~0x08));
		}
	}
}

static void spl0601_start_pressure(const struct device *dev)
{
	spl0601_write(dev, 0x08, 0x01);
}

static void spl0601_start_temperature(const struct device *dev)
{
	spl0601_write(dev, 0x08, 0x02);
}

#ifdef USE_CONTINUOUS_MODE
static void spl0601_start_continuous(const struct device *dev, uint8_t mode)
{
	spl0601_write(dev, 0x08, mode + 4);
}

static void spl0601_stop(const struct device *dev)
{
	spl0601_write(dev, 0x08, 0);
}
#endif

static int spl0601_get_raw_temp(const struct device *dev)
{
	struct spl0601_data *data = dev->data;
	uint8_t buf[3];

	int ret = spl0601_read_byte(dev, 0x03, buf, 3);

	data->raw_temperature = (int32_t)buf[0] << 16 | (int32_t)buf[1] << 8 | (int32_t)buf[2];
	data->raw_temperature = (data->raw_temperature & 0x800000) ?
					(0xFF000000 | data->raw_temperature) :
					data->raw_temperature;

	return ret;
}

static int spl0601_get_raw_pressure(const struct device *dev)
{
	struct spl0601_data *data = dev->data;
	uint8_t buf[3];

	int ret = spl0601_read_byte(dev, 0x00, buf, 3);

	data->raw_pressure = (int32_t)buf[0] << 16 | (int32_t)buf[1] << 8 | (int32_t)buf[2];
	data->raw_pressure = (data->raw_pressure & 0x800000) ? (0xFF000000 | data->raw_pressure) :
							       data->raw_pressure;

	return ret;
}

static void spl0601_get_temperature(const struct device *dev)
{
	struct spl0601_data *data = dev->data;
	float fTsc;

	fTsc = data->raw_temperature / (float)data->i32kT;

	data->calc_temperature = data->c0 * 0.5 + data->c1 * fTsc;

	return;
}

static void spl0601_get_pressure(const struct device *dev)
{
	struct spl0601_data *data = dev->data;
	float fTsc, fPsc;
	float qua2, qua3;

	fTsc = data->raw_temperature / (float)data->i32kT;
	fPsc = data->raw_pressure / (float)data->i32kP;
	qua2 = data->c10 + fPsc * (data->c20 + fPsc * data->c30);
	qua3 = fTsc * fPsc * (data->c11 + fPsc * data->c21);

	data->calc_pressure = data->c00 + fPsc * qua2 + fTsc * data->c01 + qua3;

	return;
}

static int spl0601_chip_init(const struct device *dev)
{
	struct spl0601_data *data = dev->data;

	uint8_t id = spl0601_get_device_id(dev);
	if (id != SPL0601_DEFAULT_ID) {
		LOG_ERR("Check spl0601 id failed! Get id: 0x%02x", id);
		return -EIO;
	}

	spl0601_get_calib_param(dev);

	// sampling rate = 32Hz; Pressure oversample = 8;
	spl0601_rateset(dev, SPL0601_PRESSURE_SENSOR, SPL0601_SAMPLING_RATE, OV_8);
	// sampling rate = 32Hz; Temperature oversample = 8;
	spl0601_rateset(dev, SPL0601_TEMPERATURE_SENSOR, SPL0601_SAMPLING_RATE, OV_8);

#ifndef USE_CONTINUOUS_MODE
	data->wait_time = spl0601_get_measurement_time(OV_8);
	if (data->wait_time == 0) {
		return -EIO;
	}
#else
	spl0601_start_continuous(dev, SPL0601_CONTINUOUS_P_AND_T);
	k_sleep(K_MSEC(100));
	spl0601_stop(dev);
#endif
	LOG_DBG("spl0601_init done. Chip id = 0x%x", id);

	return 0;
}

static int spl0601_init(const struct device *dev)
{
	const struct spl0601_config *config = dev->config;
	struct spl0601_data *data = dev->data;

	/* Get the I2C device */
	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	k_sleep(K_MSEC(50));

	if (spl0601_chip_init(dev) < 0) {
		return -EINVAL;
	}

	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);
	k_sem_give(&data->sem);

	return 0;
}

static int spl0601_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct spl0601_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	// Get temperature first, because calculating pressure needs temperature value
	spl0601_start_temperature(dev);

	// Wait for spl0601 get temperature
	k_sleep(K_MSEC(data->wait_time));

	// After setting 'pressure mode', spl0601 needs 40ms to initialize
	spl0601_start_pressure(dev);
	k_sleep(K_MSEC(data->wait_time + 40));

	ret = spl0601_get_raw_temp(dev);
	if (ret != 0) {
		LOG_ERR("spl0601_get_raw_temp failed!");
		return -EINVAL;
	}

	ret = spl0601_get_raw_pressure(dev);
	if (ret != 0) {
		LOG_ERR("spl0601_get_raw_pressure failed!");
		return -EINVAL;
	}

	spl0601_get_temperature(dev);
	spl0601_get_pressure(dev);

	// FIXME: If use LOG_DBG, the value will be 0. Because before log output, this func will be return.
	// LOG_DBG("SPL0601 Temp: %f Cel , Pres: %f Pa\n", data->calc_temperature, data->calc_pressure);
	// printf("SPL0601 Temp: %f Cel , Pres: %f Pa\n", data->calc_temperature, data->calc_pressure);

	k_sem_give(&data->sem);

	return ret;
}

static int spl0601_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct spl0601_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = (int32_t)data->calc_temperature;
		val->val2 = (int32_t)((data->calc_temperature - (int32_t)data->calc_temperature) *
				      1000000);
		break;
	case SENSOR_CHAN_PRESS:
		val->val1 = data->calc_pressure / 1000;
		val->val2 = ((int32_t)(data->calc_pressure * 1000) % (1000 * 1000));
		break;
	default:
		ret = -ENOTSUP;
	}

	k_sem_give(&data->sem);

	return ret;
}

static const struct sensor_driver_api spl0601_driver_api = {
	.sample_fetch = spl0601_sample_fetch,
	.channel_get = spl0601_channel_get,
};

#define SPL0601_CONFIG(inst)						       \
	{								       \
		.i2c = I2C_DT_SPEC_INST_GET(inst),			       \
	}

#define SPL0601_DEFINE(inst)						\
	static struct spl0601_data spl0601_data_##inst;			\
	static struct spl0601_config spl0601_config_##inst =		\
		SPL0601_CONFIG(inst);					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			      spl0601_init,				\
			      NULL,					\
			      &spl0601_data_##inst,			\
			      &spl0601_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &spl0601_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPL0601_DEFINE)