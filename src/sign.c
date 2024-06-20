/****************************************************************************
**
** Copyright 2024 Jiamu Sun
** Contact: barroit@linux.com
**
** This file is part of livaut.
**
** livaut is free software: you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation, either version 3 of the License, or (at your
** option) any later version.
**
** livaut is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with livaut. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "driver/i2c_master.h"
#include "helper.h"

#define I2C_MASTER_PORT   I2C_NUM_0
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21

#define PCF8574_ADDR 0x20
#define SCL_SPEED    100000

#define SGTAG "sign_init"

int sign_status = -1;

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t pcf8574;

static inline bool is_sign_avial(void)
{
	return sign_status == 0;
}

static esp_err_t init_bus_handle(i2c_master_bus_handle_t *handle)
{
	FIELD_TYPEOF(i2c_master_bus_config_t, flags) flag = {
		.enable_internal_pullup = true,
	};

	i2c_master_bus_config_t config = {
		.clk_source        = I2C_CLK_SRC_DEFAULT,
		.i2c_port          = I2C_MASTER_PORT,
		.scl_io_num        = I2C_MASTER_SCL_IO,
		.sda_io_num        = I2C_MASTER_SDA_IO,
		.glitch_ignore_cnt = 7,
		.flags             = flag,
	};

	return i2c_new_master_bus(&config, handle);
}

static esp_err_t init_dev_handle(i2c_master_bus_handle_t bus_handle,
				 i2c_master_dev_handle_t *dev_handle)
{
	i2c_device_config_t dev_config = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address  = PCF8574_ADDR,
		.scl_speed_hz    = SCL_SPEED,
	};

	return i2c_master_bus_add_device(bus_handle, &dev_config, dev_handle);
}

int init_sign(void)
{
	i2c_master_bus_handle_t bus_handle;
	i2c_master_dev_handle_t dev_handle;

	if (ROE_ESP(init_bus_handle(&bus_handle), SGTAG))
		goto err_init_bus_handle;
	else if (ROE_ESP(init_dev_handle(bus_handle, &dev_handle), SGTAG))
		goto err_init_dev_handle;

	i2c_bus = bus_handle;
	pcf8574 = dev_handle;

	sign_status = 0;

	return 0;

err_init_dev_handle:
	i2c_del_master_bus(bus_handle);
err_init_bus_handle:
	return 1;
}
