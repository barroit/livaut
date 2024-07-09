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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "termio.h"
#include "sign.h"
#include "rmt.h"
#include "driver/gpio.h"
#include "calc.h"
#include "esp_timer.h"
#include <string.h>

/* fuck these damn long name */
#define rmt_rx_register_callback rmt_rx_register_event_callbacks

static rmt_channel_handle_t rx_channel;
static QueueHandle_t incoming_symbols;

static rmt_symbol_word_t rmt_symbols[RMT_MEMORY_BLOCK_SIZE];
static rmt_receive_config_t rmt_config;

static int receive_result;

static u64 frame_interval[2];
static int is_interval_set[2];
static u8 next_interv_idx;

#define INTERVAL_TOLERANCE 8270

#define TAG "receive_signal"

static bool IRAM_ATTR copy_received_frame(rmt_channel_handle_t /* channel */,
					  const rmt_rx_done_event_data_t *syms,
					  void *ctx)
{
	frame_interval[next_interv_idx] = esp_timer_get_time();

	BaseType_t unblk;

	/* xQueueSendFromISR returns bool  */
	if (!xQueueSendFromISR(ctx, syms, &unblk))
		receive_result = -1;
	else
		receive_result = rmt_receive(rx_channel, rmt_symbols,
					     sizeof(rmt_symbols), &rmt_config);

	return unblk;
}

static void IRAM_ATTR receive_first_signal(void *)
{
	u8 idx = next_interv_idx;
	u64 intv = frame_interval[idx];
	if (intv) {
		frame_interval[idx] = esp_timer_get_time() - intv;
		is_interval_set[idx] = 1;
		next_interv_idx = !idx;
	}

	gpio_intr_disable(CONFIG_RMT_RX_GPIO);
}

static rmt_rx_channel_config_t get_chan_conf(void)
{
	rmt_rx_channel_config_t conf = {
		.gpio_num          = CONFIG_RMT_RX_GPIO,
		.clk_src           = RMT_CLOCK_SOURCE,
		.resolution_hz     = RMT_CLOCK_RESOLUTION,
		.mem_block_symbols = RMT_MEMORY_BLOCK_SIZE,
	};

	return conf;
}

static rmt_rx_event_callbacks_t get_cb_conf(void)
{
	rmt_rx_event_callbacks_t conf = {
		.on_recv_done = copy_received_frame,
	};

	return conf;
}

static int setup_channel(void)
{
	int err;

	rmt_rx_channel_config_t chan_conf = get_chan_conf();
	err = CE(rmt_new_rx_channel(&chan_conf, &rx_channel));
	if (err)
		return 1;

	rmt_rx_event_callbacks_t cb_conf = get_cb_conf();
	CE(rmt_rx_register_callback(rx_channel, &cb_conf, incoming_symbols));
	if (err)
		return 1;

	err = CE(rmt_enable(rx_channel));
	if (err)
		return 1;

	return 0;
}

static int setup_gpio_intr(void)
{
	int err;

	err = CE(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
	if (err)
		return 1;

	err = CE(gpio_isr_handler_add(CONFIG_RMT_RX_GPIO, receive_first_signal, NULL));
	if (err)
		return 1;

	gpio_set_intr_type(CONFIG_RMT_RX_GPIO, GPIO_INTR_NEGEDGE);

	gpio_intr_enable(CONFIG_RMT_RX_GPIO);

	return 0;
}

int receive_signal_setup(void)
{
	int err;

	incoming_symbols = xQueueCreate(8, sizeof(rmt_rx_done_event_data_t));

	err = setup_channel();
	if (err)
		return 1;

	err = setup_gpio_intr();
	if (err)
		return 1;

	make_aeha_receiver_config(&rmt_config);

	err = CE(rmt_receive(rx_channel, rmt_symbols,
			     sizeof(rmt_symbols), &rmt_config));
	if (err)
		return 1;

	return 0;
}

static void reset_frame_interval(void)
{
	memset(frame_interval, 0, sizeof(frame_interval));
	memset(is_interval_set, 0, sizeof(is_interval_set));
	next_interv_idx = 0;
}

int receive_signal_teardown(void)
{
	int err;

	err = CE(rmt_disable(rx_channel));
	if (err)
		return 1;
	err = CE(rmt_del_channel(rx_channel));
	if (err)
		return 1;

	gpio_intr_disable(CONFIG_RMT_RX_GPIO);

	err = CE(gpio_isr_handler_remove(CONFIG_RMT_RX_GPIO));
	if (err)
		return 1;

	gpio_uninstall_isr_service();

	vQueueDelete(incoming_symbols);

	reset_frame_interval();

	return 0;
}

static void log_frame_interval(void)
{
	u8 idx = !next_interv_idx;
	info("receive_signal()",
	     "interval between two frames is %" PRIu64 "ms",
	     (frame_interval[idx] + INTERVAL_TOLERANCE) / 1000);

	frame_interval[idx] = 0;
	is_interval_set[idx] = 0;
}

static void print_signals(u8 *bitmap, size_t n)
{
	if (n != 0) {
		print_frame_dump(bitmap, n);
		putchar('\n');

		fflush(stdout);
		free(bitmap);
	}
}

enum action_result receive_signal(void)
{
	if (receive_result) {
		error(TAG,
		      "ISR has an error occurred (code %d)", receive_result);
		return EXEC_ERROR;
	}

	rmt_rx_done_event_data_t data;
	if (!xQueueReceive(incoming_symbols, &data, pdMS_TO_TICKS(1500))) {
		static u8 sign = SIGN_1 | SIGN_3 | SIGN_5 | SIGN_7;
		reset_frame_interval();
		show_sign(sign);
		sign ^= 0xFF;
		return EXEC_RETRY;
	}

	gpio_intr_enable(CONFIG_RMT_RX_GPIO);

	u8 *signals;
	size_t sigsz;
	switch (decode_aeha_symbols(data.received_symbols,
		data.num_symbols, &signals, &sigsz)) {
	case DEC_SKIP:
		reset_frame_interval();
		return EXEC_RETRY;
	case DEC_DONE:
		if (is_interval_set[!next_interv_idx])
			log_frame_interval();
		print_signals(signals, sigsz);
		show_sign(SIGN_ON);
		/**
		 * since some remote controllers send frame multiple times, we
		 * need to ensure the next frame is processed by decoder, so
		 * return ACTION_RETY to avoid frame loss
		 */
		return EXEC_RETRY;
	case DEC_ERROR:
		return EXEC_ERROR;
	}

	return 0; /* make gcc happy */
}
