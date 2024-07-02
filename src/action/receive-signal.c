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

#define RS_ ESP_ERROR_CHECK /* alias */

#define CH_CLOCK_RES   1000000 /* channel clock resolution */
#define CH_CLOCK_SRC   RMT_CLK_SRC_DEFAULT
#define CH_MEMBLK_SIZE 256

#define BUFFER_SIZE 256

static rmt_channel_handle_t rx_channel;
static QueueHandle_t incoming_symbols;
static rmt_symbol_word_t rmt_symbols[BUFFER_SIZE];
static rmt_receive_config_t rmt_config;
static int receive_result;

static void install_rx_channel(void)
{
	rmt_rx_channel_config_t conf = {
		.gpio_num          = GPIO_NUM_19,
		.clk_src           = CH_CLOCK_SRC,
		.resolution_hz     = CH_CLOCK_RES,
		.mem_block_symbols = CH_MEMBLK_SIZE,
	};

	RS_(rmt_new_rx_channel(&conf, &rx_channel));
}

static bool receive_frame(rmt_channel_handle_t _, /* rx channel */
			  const rmt_rx_done_event_data_t *syms,
			  void *ctx)
{
	BaseType_t unblk;

	/* xQueueSendFromISR returns bool  */
	if (!xQueueSendFromISR(ctx, syms, &unblk))
		receive_result = -1;
	else
		receive_result = rmt_receive(rx_channel, rmt_symbols,
					     sizeof(rmt_symbols), &rmt_config);

	return unblk;
}

int receive_signal_setup(struct action_config *conf)
{
	install_rx_channel();

	rmt_rx_event_callbacks_t cb = {
		.on_recv_done = receive_frame,
	};

	incoming_symbols = xQueueCreate(8, sizeof(rmt_rx_done_event_data_t));

	RS_(rmt_rx_register_event_callbacks(rx_channel, &cb,
					    incoming_symbols));

	RS_(rmt_enable(rx_channel));

	make_aeha_receiver_config(&rmt_config);

	conf->delay = 0; /* we apply delay from receive_signal() */
	info("receive_signal_setup()", "ok");

	RS_(rmt_receive(rx_channel, rmt_symbols,
			sizeof(rmt_symbols), &rmt_config));
	return 0;
}

int receive_signal_teardown(void)
{
	RS_(rmt_disable(rx_channel));
	RS_(rmt_del_channel(rx_channel));

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

enum action_state receive_signal(void)
{
	if (receive_result) {
		error("receive_frame()",
		      "ISR has an error occurred (code %d)", receive_result);
		return ACT_ERRO;
	}

	rmt_rx_done_event_data_t data;
	if (!xQueueReceive(incoming_symbols, &data, pdMS_TO_TICKS(1000))) {
		return ACT_AGIN;
	}

	u8 *signals;
	size_t sigsz;
	switch (decode_aeha_symbols(data.received_symbols,
		data.num_symbols, &signals, &sigsz)) {
	case DEC_DONE:
		show_signal_info(signals, sigsz);
		free(signals);
		show_sign(SN_ON);
		/* FALLTHRU */
	case DEC_SKIP:
		return ACT_RETY;
	case DEC_ERRO:
		return ACT_ERRO;
	}

	return 0; /* make gcc happy */
}
