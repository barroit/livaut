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
#include "sign.h"
#include "debug.h"
#include "jumper.h"
#include "execute-action.h"
#include "soc/gpio_num.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include "nvs.h"
#include "sntp.h"
#include "power.h"

ACTION_DECLARATION(schedule_signal);
ACTION_DECLARATION(receive_signal);

static const struct action actions[] = {
	ACT(schedule_signal,   GPIO_NUM_32, GPIO_NUM_26),
	ACT(receive_signal, GPIO_NUM_32, GPIO_NUM_25),
	ACT_END(),
};

void at_sta2ap_connect(int err)
{
	if (err) {
		release_nvs_flash();
	} else {
		config_sntp_service();
		start_sntp_service();
	}
}

void app_main(void)
{
	int err;

	err = init_nvs_flash();
	if (err)
		goto setup_jumper;

	collaborate_timezone();

	xTaskCreate(make_sta2ap_connection, "wifi_setup", 4096,
		    at_sta2ap_connect, 5, NULL);

setup_jumper:
	err = config_jumper();
	if (err)
		start_deep_sleep();

	err = install_mst_bus();
	if (err)
		goto do_action;

	debugging()
		bus_dev_scan_7bit();

	err = init_sign();
	if (err)
		uninstall_master_bus();

do_action:
	xTaskCreate(execute_action, "action_exec", 4096,
		    (void *)actions, 15, NULL);
}
