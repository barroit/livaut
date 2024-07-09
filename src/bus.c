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

#include "bus.h"
#include "driver/i2c_master.h"
#include "types.h"
#include "termio.h"
#include "list.h"

#define TAG "master_bus"

#define I2C_GLITCH_THOLD 7

static i2c_master_bus_handle_t master_bus;

int install_mst_bus(void)
{
	int err;

	FIELD_TYPEOF(i2c_master_bus_config_t, flags) flag = {
		.enable_internal_pullup = true,
	};

	i2c_master_bus_config_t conf = {
		.clk_source        = I2C_CLK_SRC_DEFAULT,
		.i2c_port          = I2C_NUM_0,
		.scl_io_num        = CONFIG_MASTER_BUS_SCL,
		.sda_io_num        = CONFIG_MASTER_BUS_SDA,
		.glitch_ignore_cnt = I2C_GLITCH_THOLD,
		.flags             = flag,
	};

	err = CE(i2c_new_master_bus(&conf, &master_bus));
	if (err)
		return 1;

	return 0;
}

i2c_master_bus_handle_t get_master_bus(void)
{
	return master_bus;
}

void uninstall_master_bus(void)
{
	CE(i2c_del_master_bus(master_bus));
	master_bus = NULL;
}

void bus_dev_scan_7bit(void)
{
	u16 addr;
	int found = 0;

	for_each_idx(addr, 0x7F) {
		if (i2c_master_probe(master_bus, addr, 100) == ESP_OK) {
			info(TAG, "found device at 0x%02x", addr);
			found = 1;
		}
	}

	if (!found)
		info(TAG, "no device was found one the bus");
}
