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
#define CH_MEMBLK_SIZE 64

#define RX_GPIO GPIO_NUM_19
#define RX_FREQ 25000	/**
			 * for RX channel, we should not set the carrier
			 * frequency exactly to the theoretical value due to
			 * reflection and refraction occur when a signal
			 * travels through the air
			 */
#define RX_DUTY 0.33
#define RX_ONLL false /* active on low level */

/* T: 350μs ~ 500μs (425μs typ.) */
#define SIG_MINTHOLD 100000  /* bit '0': 1T + 1T > 100000nm */
#define SIG_MAXTHOLD 8000000 /* repeat:  8T + 8T < 8000000nm */

#define SYMBUF_SIZE 64

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
	QueueHandle_t que = (QueueHandle_t)ctx;

	xQueueSendFromISR(que, syms, &unblk);
	return unblk;
}

static void decode_symbol(rmt_symbol_word_t *sym)
{
	//
}

static void decode_symbols(rmt_symbol_word_t *syms, size_t n)
{
	//
}

void deploy_rx_channel(void)
{
	install_rx_channel();

	rmt_rx_event_callbacks_t cb = {
		.on_recv_done = receive_frame,
	};

	S_(rmt_rx_register_event_callbacks(rx_channel, &cb, incoming_symbols));

	S_(rmt_enable(rx_channel));
}

static void receive_symbol_step(void)
{
	rmt_symbol_word_t symbuf[SYMBUF_SIZE];
	rmt_rx_done_event_data_t evdt;
	rmt_receive_config_t cfg = {
		.signal_range_min_ns = SIG_MINTHOLD,
		.signal_range_max_ns = SIG_MAXTHOLD,
	};

	S_(rmt_receive(rx_channel, symbuf, sizeof(symbuf), &cfg));

	while (39) {
		if (!xQueueReceive(incoming_symbols, &evdt, portMAX_DELAY))
			continue;

		decode_symbols(evdt.received_symbols, evdt.num_symbols);

		S_(rmt_receive(rx_channel, symbuf, sizeof(symbuf), &cfg));
	}
}

void destroy_rx_channel(void)
{
	S_(rmt_disable(rx_channel));
	S_(rmt_del_channel(rx_channel));

	rx_channel = NULL;
}
