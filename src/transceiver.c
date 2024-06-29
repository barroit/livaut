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

#include "transceiver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "aeha-protocol.h"
#include "sign.h"
#include "term-io.h"
#include "memory.h"

#define S_ ESP_ERROR_CHECK /* alias */

#define CH_CLOCK_RES   1000000 /* channel clock resolution */
#define CH_CLOCK_SRC   RMT_CLK_SRC_DEFAULT
#define CH_MEMBLK_SIZE 256

#define RX_GPIO GPIO_NUM_19

#define BUFFER_SIZE 256

static rmt_channel_handle_t rx_channel;
static QueueHandle_t incoming_symbols;

enum receiver_state {
	RX_FTERR = -1,
	RX_DONXT,
	RX_TMOUT,
};

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

static inline void show_signal_info(const u8 *bits, size_t n)
{
	print_bit_dump(bits, n);

	/* for verify purpose */
	print_checksum(bits, n - 1);

	fflush(stdout);
}

static enum receiver_state do_receive_symbol(void)
{
	rmt_rx_done_event_data_t data;
	u8 *out;
	size_t sz;

	if (xQueueReceive(incoming_symbols, &data, pdMS_TO_TICKS(500)))
		switch (decode_aeha_symbols(data.received_symbols,
			data.num_symbols, &out, &sz)) {
		case DCD_ERR:
			return RX_FTERR;
		case DCD_NXT:
			show_sign(SN_ON);
			show_signal_info(out, sz);
			free(out);
			/* FALLTHRU */
		case DCD_RTY:
			return RX_DONXT;
		}
	else
		return RX_TMOUT;

	return ~0; /* make gcc happy */
}

void receive_symbol_step(void)
{
	rmt_symbol_word_t buf[BUFFER_SIZE];
	rmt_receive_config_t conf;
	u8 sign = SN_1 | SN_3 | SN_5 | SN_7;

	make_aeha_receiver_config(&conf);
	S_(rmt_receive(rx_channel, buf, sizeof(buf), &conf));

	while (39)
		switch (do_receive_symbol()) {
		case RX_FTERR:
			show_sign(SN_OF);
			exit(EXIT_FAILURE);
		case RX_DONXT:
			S_(rmt_receive(rx_channel, buf, sizeof(buf), &conf));
			continue;
		case RX_TMOUT:
			show_sign(sign);
			sign ^= 0xFF;
		}
}

void destroy_rx_channel(void)
{
	S_(rmt_disable(rx_channel));
	S_(rmt_del_channel(rx_channel));

	vQueueDelete(incoming_symbols);

	rx_channel = NULL;
}
