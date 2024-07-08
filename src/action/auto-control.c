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

#include "run-action.h"
#include "aeha-protocol.h"
#include "rmt.h"
#include "termio.h"
#include "signal-schedule.h"
#include "sntp.h"
#include "list.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "calc.h"

#define RS ESP_ERROR_CHECK

#define SYMBOL_BUFFER_SIZE   RMT_MBLK_SZ
#define TRANSFER_QUEUE_DEPTH 3

#define TAG "signal_schedule"

static rmt_channel_handle_t tx_channel;
static rmt_encoder_handle_t encoder;
static u8 next_schedule;

static int install_tx_channel(void)
{
	rmt_tx_channel_config_t conf = {
		.clk_src           = RMT_CLK_SRC,
		.gpio_num          = GPIO_NUM_18,
		.mem_block_symbols = RMT_MBLK_SZ,
		.resolution_hz     = RMT_CLK_RES,
		.trans_queue_depth = TRANSFER_QUEUE_DEPTH,
	};

	return rmt_new_tx_channel(&conf, &tx_channel);
}

static int configure_tx_carrier(void)
{
	FIELD_TYPEOF(rmt_carrier_config_t, flags) flag = {
		.polarity_active_low = false,
	};

	rmt_carrier_config_t conf = {
		.duty_cycle   = AEHA_DUTY_CYCLE,
		.frequency_hz = AEHA_FREQUENCY,
		.flags        = flag,
	};

	return rmt_apply_carrier(tx_channel, &conf);
}

int auto_control_setup(struct action_config *)
{
	RS(install_tx_channel());

	RS(configure_tx_carrier());

	RS(rmt_enable(tx_channel));

	RS(make_aeha_encoder(&encoder));

	info(TAG, "setup()");

	return 0;
}

int auto_control_teardown(void)
{
	RS(rmt_disable(tx_channel));

	RS(rmt_del_channel(tx_channel));

	RS(encoder->del(encoder));

	info(TAG, "teardown()");

	return 0;
}

void transmit_signal(frame_info_t *frame)
{
	rmt_transmit_config_t conf = { 0 };
	if (frame->bnum) {
		if (convert_aeha_lldat(frame->lldat, frame->bnum))
			die(TAG, "invalid lldat found");
	}

	RS(rmt_transmit(tx_channel, encoder, frame, ~0, &conf));
}

static int is_auto_controllable(void)
{
	static size_t count;

	if (!is_sntp_started()) {
		if (count++ % 5 == 0)
			warning(TAG, "sntp service is not running");
		return 0;
	}

	count = 0;
	return 1;
}

static void goto_next_schedule(void)
{
	if (next_schedule + 1 == sizeof(schedules))
		next_schedule = 0;
	else
		next_schedule++;
}

enum action_state auto_control(void)
{
	const struct signal_schedule *schedule = &schedules[next_schedule];
	u64 ts = schedule->start, now = get_seconds_of_day();

	if (!is_auto_controllable() || schedule->start > now) {
		return ACTION_AGIN;
	} else if (schedule->start < now - 5) {
		goto_next_schedule();
		info(TAG, "skipped a schedule set for " HH_MM_SS,
		     sec_to_hour_d(ts), sec_to_min_d(ts), sec_to_sec_d(ts));
		return ACTION_RETY;
	}

	size_t i;
	for_each_idx(i, schedule->fnum)
		transmit_signal(schedule->frame);

	goto_next_schedule();

	ts = schedules[next_schedule].start;
	info(TAG, "next schedule is set to run at " HH_MM_SS,
	     sec_to_hour_d(ts), sec_to_min_d(ts), sec_to_sec_d(ts));

	return ACTION_DONE;
}
