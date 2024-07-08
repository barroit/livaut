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
#include "run-action.h"
#include "soc/gpio_num.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include "nvs.h"
#include "sntp.h"

DEFINE_ACTION_SIGNATURE(auto_control);
DEFINE_ACTION_SIGNATURE(receive_signal);

static const struct action_info actions[] = {
	ACTION(auto_control,   GPIO_NUM_32, GPIO_NUM_26),
	ACTION(receive_signal, GPIO_NUM_32, GPIO_NUM_25),
	ACTION_TAIL(),
};

void setup_wifi_task_callback(int err)
{
	if (err) {
		release_nvs_flash();
		return;
	}

	err = setup_sntp_service();
	if (!err)
		start_sntp_service();
}

#include "list.h"
#include "signal-schedule-def.h"
#include "driver/rmt_tx.h"

// static const struct signal_schedule *schedule = &(struct signal_schedule){
// 	.start = 34200,
// 	.frame = (struct frame_info[]){
// 			{
// 				.delay = 0,
// 				.lldat = 0,
// 				.bnum  = 0,
// 				.cnum  = 4,
// 				.data  = (uint8_t[]){ 0x11, 0xDA, 0x27, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97 },
// 				.unum  = 16,
// 			},
// 			{
// 				.delay = 35,
// 				.lldat = 0,
// 				.bnum  = 0,
// 				.cnum  = 4,
// 				.data  = (uint8_t[]){ 0x11, 0xDA, 0x27, 0x00, 0x00, 0x38, 0x34, 0x00, 0xAF, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0xC3, 0x00, 0x00, 0x56 },
// 				.unum  = 15,
// 			},
// 	},
// 	.fnum  = 2,
// };

void transmit_signal(frame_info_t *frame);

void test(void *)
{
	// struct action_config conf = { 0 };
	// receive_signal_setup(&conf);

	// auto_control_setup(NULL);

	// size_t i;
	// for_each_idx(i, schedule->fnum) {
	// 	transmit_signal(&schedule->frame[i]);
	// }

	// while (39)
		// receive_signal();
}

void app_main(void)
{
	// xTaskCreate(test, "test", 4096, NULL, 15, NULL);
	int err;

// 	err = init_nvs_flash();
// 	if (err)
// 		goto setup_jumper;

// 	collaborate_timezone();

// 	xTaskCreate(setup_wifi_task, "wifi_init", 2048,
// 		    setup_wifi_task_callback, 10, NULL);

// setup_jumper:
	config_jumper();

	err = install_mst_bus();
	if (err)
		goto do_action;

	debugging()
		bus_dev_scan_7bit();

	err = init_sign();
	if (err)
		uninstall_mst_bus();

do_action:
	xTaskCreate(exec_action_task, "action_exec", 4096,
		    (void *)actions, 15, NULL);
}
