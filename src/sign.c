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

#define EXPANDER_ADDR 0x20
#define SCL_SPEED    100000

static i2c_master_dev_handle_t expander;

int init_sign(void)
{
	i2c_master_bus_handle_t bus = get_mst_bus();
	i2c_device_config_t conf = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address  = EXPANDER_ADDR,
		.scl_speed_hz    = SCL_SPEED,
	};

	if (i2c_master_bus_add_device(bus, &conf, &expander))
		return warning("init_sign()",
			       "failed to initialize expander (address 0x%X)",
			       EXPANDER_ADDR);

	return 0;
}

int show_sign(u8 code)
{
	if (!expander || !get_mst_bus())
		return -1;

	return i2c_master_transmit(expander, &code, 1, -1);
}
