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

#include "helper.h"
#include "bus.h"
#include "sign.h"
#include "transceiver.h"

void app_main(void)
{
	// if (init_mst_bus())
	// 	goto schedule_task;

	// bus_dev_scan_7bit();

	// if (init_sign())
	// 	dsty_mst_bus();

// schedule_task:
	deploy_rx_channel();

	// wait_ir_sig();
}
