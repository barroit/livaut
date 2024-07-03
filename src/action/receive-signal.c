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

#define RS ESP_ERROR_CHECK /* alias */

#define RX_GPIO GPIO_NUM_19

static rmt_channel_handle_t rx_channel;
static QueueHandle_t incoming_symbols;

#define SYMBOL_BUFFER_SIZE RMT_MBLK_SZ

static rmt_symbol_word_t rmt_symbols[SYMBOL_BUFFER_SIZE];
static rmt_receive_config_t rmt_config;

static int receive_result;

static u64 frame_interval[2];
static int is_interval_set[2];
static u8 next_inter_idx;

#define INTERVAL_TOLERANCE 8270

static bool IRAM_ATTR copy_received_frame(rmt_channel_handle_t /* channel */,
					  const rmt_rx_done_event_data_t *syms,
					  void *ctx)
{
	frame_interval[next_inter_idx] = esp_timer_get_time();

	BaseType_t unblk;

	/* xQueueSendFromISR returns bool  */
	if (!xQueueSendFromISR(ctx, syms, &unblk))
		receive_result = -1;
	else
		receive_result = rmt_receive(rx_channel, rmt_symbols,
					     sizeof(rmt_symbols), &rmt_config);

	return unblk;
}

static void init_rx_channel(void)
{
	rmt_rx_channel_config_t conf = {
		.gpio_num          = RX_GPIO,
		.clk_src           = RMT_CLK_SRC,
		.resolution_hz     = RMT_CLK_RES,
		.mem_block_symbols = RMT_MBLK_SZ,
	};

	RS(rmt_new_rx_channel(&conf, &rx_channel));

	rmt_rx_event_callbacks_t cb = {
		.on_recv_done = copy_received_frame,
	};

	RS(rmt_rx_register_event_callbacks(rx_channel, &cb,
					   incoming_symbols));

	RS(rmt_enable(rx_channel));
}

static void IRAM_ATTR receive_first_signal(void *)
{
	u8 idx = next_inter_idx;
	if (frame_interval[idx]) {
		frame_interval[idx] = esp_timer_get_time() -
				      frame_interval[idx];
		is_interval_set[idx] = 1;
		next_inter_idx = !idx;
	}

	gpio_intr_disable(RX_GPIO);
}

static void config_rx_gpio(void)
{
	RS(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));

	RS(gpio_isr_handler_add(RX_GPIO, receive_first_signal, NULL));

	gpio_set_intr_type(RX_GPIO, GPIO_INTR_NEGEDGE);

	gpio_intr_enable(RX_GPIO);
}

int receive_signal_setup(struct action_config *conf)
{
	conf->delay = 0; /* we apply delay from receive_signal() */

	incoming_symbols = xQueueCreate(8, sizeof(rmt_rx_done_event_data_t));

	init_rx_channel();

	config_rx_gpio();

	make_aeha_receiver_config(&rmt_config);

	RS(rmt_receive(rx_channel, rmt_symbols,
		       sizeof(rmt_symbols), &rmt_config));

	info("receive_signal_setup()", "ok");
	return 0;
}

int receive_signal_teardown(void)
{
	RS(rmt_disable(rx_channel));
	RS(rmt_del_channel(rx_channel));

	vQueueDelete(incoming_symbols);

	info("receive_signal_teardown()", "ok");
	return 0;
}

static inline void show_signal_info(const u8 *bits, size_t n)
{
	print_bit_dump(bits, n);

	/* for verify purpose */
	print_checksum(bits, n - 1);

	fflush(stdout);
}

static void log_frame_interval(void)
{
	u8 idx = !next_inter_idx;
	info("receive_signal()",
	     "interval between two frames is %" PRIu64 "ms",
	     (frame_interval[idx] + INTERVAL_TOLERANCE) / 1000);

	frame_interval[idx] = 0;
	is_interval_set[idx] = 0;
}

static void reset_frame_interval(void)
{
	memset(frame_interval, 0, sizeof(frame_interval));
	memset(is_interval_set, 0, sizeof(is_interval_set));
	next_inter_idx = 0;
}

enum action_state receive_signal(void)
{
	if (receive_result) {
		error("copy_received_frame()",
		      "ISR has an error occurred (code %d)", receive_result);
		return ACTION_ERRO;
	}

	rmt_rx_done_event_data_t data;
	if (!xQueueReceive(incoming_symbols, &data, pdMS_TO_TICKS(1000))) {
		reset_frame_interval();
		return ACTION_AGIN;
	}

	gpio_intr_enable(RX_GPIO);

	u8 *signals;
	size_t sigsz;
	switch (decode_aeha_symbols(data.received_symbols,
		data.num_symbols, &signals, &sigsz)) {
	case DEC_DONE:
		if (is_interval_set[!next_inter_idx])
			log_frame_interval();

		show_signal_info(signals, sigsz);
		free(signals);
		show_sign(SN_ON);
		/* FALLTHRU */
	case DEC_SKIP:
		return ACTION_RETY;
	case DEC_ERRO:
		return ACTION_ERRO;
	}

	return 0; /* make gcc happy */
}
