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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "convert-data.h"
#include "aeha-protocol.h"
#include <string.h>
#include "sign.h"

#define S_ ESP_ERROR_CHECK /* alias */

#define CH_CLOCK_RES   1000000 /* channel clock resolution */
#define CH_CLOCK_SRC   RMT_CLK_SRC_DEFAULT
#define CH_MEMBLK_SIZE 256

#define RX_GPIO GPIO_NUM_19

#define BUFFER_SIZE 256

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

static void fprint_autowrap(const char *pfx, size_t wrap,
			    const char *txt, FILE *stream)
{
	size_t pfxlen = strlen(pfx), txtlen = strlen(txt), len = pfxlen;

	/* usually ‘pfx’ will not exceed ‘wrap’ */
	fputs(pfx, stream);

	while (39) {
		size_t room = wrap - len;

		fprintf(stream, "%.*s\n", (int)room, txt);

		if (txtlen <= room)
			break;

		txt += room;
		txtlen -= room;
		len = fprintf(stream, "%*s", (int)pfxlen, "");
	}
}

#define print_autowrap(p, w, t) fprint_autowrap(p, w, t, stdout);

static void show_signal(const u8 *buf, size_t sz)
{
	char hexstr[bit_size_to_hex_size(BUFFER_SIZE)];
	char bitstr[BUFFER_SIZE];

	bit_arr_to_hex_str_lsb(buf, sz, hexstr);
	bit_arr_to_bit_str(buf, sz, bitstr);

	print_autowrap("bin: ", 80, bitstr);
	print_autowrap("hex: ", 80, hexstr);

	putchar('\n');

	fflush(stdout);
}

static int do_receive_symbol(rmt_symbol_word_t *symbuf, size_t sbsz,
			     rmt_receive_config_t *cfg, u8 *sign)
{
	rmt_rx_done_event_data_t data;
	u8 buf[BUFFER_SIZE];
	size_t sz;

	if (xQueueReceive(incoming_symbols, &data, pdMS_TO_TICKS(500))) {
		switch (decode_aeha_symbols(data.received_symbols,
			data.num_symbols, buf, &sz)) {
		case DCD_ERR:
			return 1;
		case DCD_NXT:
			show_signal(buf, sz);
			show_sign(SN_ON);
		case DCD_RTY:
			/* make gcc happy */
		}

		S_(rmt_receive(rx_channel, symbuf, sbsz, cfg));
	} else {
		show_sign(*sign);
		*sign ^= 0xFF;
	}

	return 0;
}

void receive_symbol_step(void)
{
	rmt_symbol_word_t symbuf[BUFFER_SIZE];
	rmt_receive_config_t cfg;
	u8 sign = SN_1 | SN_3 | SN_5 | SN_7;

	make_aeha_receiver_config(&cfg);
	S_(rmt_receive(rx_channel, symbuf, sizeof(symbuf), &cfg));

	while (39) {
		if (do_receive_symbol(symbuf, sizeof(symbuf), &cfg, &sign)) {
			show_sign(SN_OF);
			break;
		}
	}
}

void destroy_rx_channel(void)
{
	S_(rmt_disable(rx_channel));
	S_(rmt_del_channel(rx_channel));

	vQueueDelete(incoming_symbols);

	rx_channel = NULL;
}
