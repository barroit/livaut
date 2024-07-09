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

#include "execute-action.h"
#include "aeha-protocol.h"
#include "rmt.h"
#include "termio.h"
#include "signal-schedule.h"
#include "sntp.h"
#include "list.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "calc.h"

#define CE ESP_ERROR_CHECK_WITHOUT_ABORT

#define SYMBOL_BUFFER_SIZE   RMT_MBLK_SZ
#define TRANSFER_QUEUE_DEPTH 4

#define TAG "signal_schedule"

static rmt_channel_handle_t tx_channel;
static rmt_encoder_handle_t encoder;
static u8 next_schedule;

static rmt_tx_channel_config_t get_chan_conf(void)
{
	rmt_tx_channel_config_t conf = {
		.clk_src           = RMT_CLK_SRC,
		.gpio_num          = GPIO_NUM_18,
		.mem_block_symbols = RMT_MBLK_SZ,
		.resolution_hz     = RMT_CLK_RES,
		.trans_queue_depth = TRANSFER_QUEUE_DEPTH,
	};

	return conf;
}

static rmt_carrier_config_t get_carr_conf(void)
{
	FIELD_TYPEOF(rmt_carrier_config_t, flags) flag = {
		.polarity_active_low = false,
	};

	rmt_carrier_config_t conf = {
		.duty_cycle   = AEHA_DUTY_CYCLE,
		.frequency_hz = AEHA_FREQUENCY,
		.flags        = flag,
	};

	return conf;
}

int schedule_signal_setup(void)
{
	int err;

	rmt_tx_channel_config_t chan_conf = get_chan_conf();
	err = CE(rmt_new_tx_channel(&chan_conf, &tx_channel));
	if (err)
		return 1;

	rmt_carrier_config_t carr_conf = get_carr_conf();
	err = CE(rmt_apply_carrier(tx_channel, &carr_conf));
	if (err)
		return 1;

	err = CE(rmt_enable(tx_channel));
	if (err)
		return 1;

	err = CE(make_aeha_encoder(&encoder));
	if (err)
		return 1;

	return 0;
}

int schedule_signal_teardown(void)
{
	int err;

	err = CE(rmt_disable(tx_channel));
	if (err)
		return 1;

	err = CE(rmt_del_channel(tx_channel));
	if (err)
		return 1;

	err = CE(encoder->del(encoder));
	if (err)
		return 1;

	next_schedule = 0;

	return 0;
}

static int transmit_signal(frame_info_t *frame)
{
	int err = 0;
	rmt_transmit_config_t conf = { 0 };

	if (frame->bnum && !is_lldat_converted(frame->lldat))
		err = convert_aeha_lldat(frame->lldat, frame->bnum);

	if (err)
		die(TAG, "invalid lldat found");

	err = rmt_transmit(tx_channel, encoder, frame, ~0, &conf);
	if (err)
		return 1;

	return 0;
}

static int is_schedule_signallable(void)
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

static void move_schedule_next(void)
{
	if (next_schedule + 1 == sizeof(schedules))
		next_schedule = 0;
	else
		next_schedule++;
}

enum action_result schedule_signal(void)
{
	const struct signal_schedule *schedule = &schedules[next_schedule];
	u64 ts = schedule->start, now = get_seconds_of_day();

	if (!is_schedule_signallable() || schedule->start > now) {
		return EXEC_AGAIN;
	} else if (schedule->start < now - 5) {
		move_schedule_next();
		info(TAG, "skipped a schedule set for " HH_MM_SS,
		     sec_to_hour_d(ts), sec_to_min_d(ts), sec_to_sec_d(ts));
		return EXEC_RETRY;
	}

	size_t i;
	int err;
	for_each_idx(i, schedule->fnum) {
		err = transmit_signal(schedule->frame);
		if (err)
			return EXEC_ERROR;
	}

	move_schedule_next();

	ts = schedules[next_schedule].start;
	info(TAG, "next schedule is set to run at " HH_MM_SS,
	     sec_to_hour_d(ts), sec_to_min_d(ts), sec_to_sec_d(ts));

	return EXEC_AGAIN;
}
