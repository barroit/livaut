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

#include <stdbool.h>
#include "transceiver.h"
#include "driver/rmt_rx.h"
#include "helper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define S_ ESP_ERROR_CHECK /* alias */

#define CH_CLOCK_RES   1000000 /* channel clock resolution */
#define CH_CLOCK_SRC   RMT_CLK_SRC_DEFAULT
#define CH_MEMBLK_SIZE 256

#define RX_GPIO GPIO_NUM_19

/**
 * for AEHA format
 * time unit: 350μs ~ 500μs (425μs typ.)
 * bit '0':   1T + 1T
 * repreat:   8T + 8T
 */
#define SIG_MINTHOLD 1250    /* bit ‘0’ > 1250nm */
#define SIG_MAXTHOLD 8000000 /* repreat < 8000000nm */

#define SYMBUF_SIZE 256

/**
 * we will receive 162 (168 total) symbols from リモコン arc478a78
 */
#define VALID_FRAMES 162

static rmt_channel_handle_t rx_channel;
static QueueHandle_t incoming_symbols;

static void install_rx_channel(void)
{
	rmt_rx_channel_config_t cfg = {
		.gpio_num          = RX_GPIO,
		.clk_src           = CH_CLOCK_SRC,
		.resolution_hz     = CH_CLOCK_RES,
		.mem_block_symbols = CH_MEMBLK_SIZE,
	};

	S_(rmt_new_rx_channel(&cfg, &rx_channel));
}

static bool receive_frame(rmt_channel_handle_t _, /* rx channel */
			  const rmt_rx_done_event_data_t *syms,
			  void *ctx)
{
	BaseType_t unblk;

	xQueueSendFromISR(ctx, syms, &unblk);
	return unblk;
}

static void decode_symbol(rmt_symbol_word_t *sym)
{
	// do stuff...
}

static inline bool is_valid_frame(size_t n)
{
	return n == VALID_FRAMES;
}

static void decode_symbol_entries(rmt_symbol_word_t *syms, size_t n)
{
	if (is_valid_frame(n)) {
		size_t i;

		for_each_idx(i, n)
			decode_symbol(&syms[i]);
	}		
}

void deploy_rx_channel(void)
{
	install_rx_channel();

	rmt_rx_event_callbacks_t cb = {
		.on_recv_done = receive_frame,
	};

	incoming_symbols = xQueueCreate(8, sizeof(rmt_rx_done_event_data_t));

	S_(rmt_rx_register_event_callbacks(rx_channel, &cb, incoming_symbols));

	S_(rmt_enable(rx_channel));
}

void receive_symbol_step(void)
{
	rmt_symbol_word_t symbuf[SYMBUF_SIZE];
	rmt_rx_done_event_data_t evt;
	rmt_receive_config_t cfg = {
		.signal_range_min_ns = SIG_MINTHOLD,
		.signal_range_max_ns = SIG_MAXTHOLD,
	};

	S_(rmt_receive(rx_channel, symbuf, sizeof(symbuf), &cfg));

	while (39) {
		if (!xQueueReceive(incoming_symbols, &evt, portMAX_DELAY))
			continue;

		decode_symbol_entries(evt.received_symbols, evt.num_symbols);

		S_(rmt_receive(rx_channel, symbuf, sizeof(symbuf), &cfg));
	}
}

void destroy_rx_channel(void)
{
	S_(rmt_disable(rx_channel));
	S_(rmt_del_channel(rx_channel));

	vQueueDelete(incoming_symbols);

	rx_channel = NULL;
}
