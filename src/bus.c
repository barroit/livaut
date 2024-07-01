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

#define I2C_MASTER_SCL   22
#define I2C_MASTER_SDA   21
#define I2C_GLITCH_THOLD 7

static i2c_master_bus_handle_t mst_bus;

int install_mst_bus(void)
{
	FIELD_TYPEOF(i2c_master_bus_config_t, flags) flag = {
		.enable_internal_pullup = true,
	};

	i2c_master_bus_config_t conf = {
		.clk_source        = I2C_CLK_SRC_DEFAULT,
		.i2c_port          = I2C_NUM_0,
		.scl_io_num        = I2C_MASTER_SCL,
		.sda_io_num        = I2C_MASTER_SDA,
		.glitch_ignore_cnt = I2C_GLITCH_THOLD,
		.flags             = flag,
	};

	if (i2c_new_master_bus(&conf, &mst_bus))
		return warning("install_mst_bus()", "failed to initialize bus");

	return 0;
}

i2c_master_bus_handle_t get_mst_bus(void)
{
	return mst_bus;
}

void uninstall_mst_bus(void)
{
	i2c_del_master_bus(mst_bus);
	mst_bus = NULL;
}

void bus_dev_scan_7bit(void)
{
	u16 addr;
	int found = 0;

	for_each_idx(addr, 0x7F) {
		if (i2c_master_probe(mst_bus, addr, 100) == ESP_OK) {
			info("dev_scan", "found device at 0x%02x", addr);
			found = 1;
		}
	}

	if (!found)
		info("dev_scan", "no device was found one the bus");
}
