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

#include "sign.h"
#include "bus.h"
#include "termio.h"
#include "driver/i2c_master.h"
#include "types.h"

#define SCL_SPEED 100000

static i2c_master_dev_handle_t expander;

int init_sign(void)
{
	int err;

	i2c_master_bus_handle_t bus = get_master_bus();
	if (!bus)
		return 1;

	i2c_device_config_t conf = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address  = CONFIG_EXPANDER_ADDRESS,
		.scl_speed_hz    = SCL_SPEED,
	};

	err = CE(i2c_master_bus_add_device(bus, &conf, &expander));
	if (err)
		return 1;

	return 0;
}

int show_sign(u8 code)
{
	int err;

	if (!expander || !get_master_bus())
		return -1;

	err = i2c_master_transmit(expander, &code, 1, -1);
	if (err)
		return 1;

	return 0;
}
