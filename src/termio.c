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

#include "termio.h"
#include <stdio.h>
#include <inttypes.h>
#include "list.h"
#include "strbuf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define bit_pos_mask(a) (a % 8)
#define is_byte_head(a) (bit_pos_mask(a) == 0)
#define is_byte_tail(a) (bit_pos_mask(a) == 7)
#define is_byte_midd(a) (bit_pos_mask(a) == 3)

static void do_print_bit_dump(struct strbuf *sb, const u8 *bitmap, size_t i)
{
	if (is_byte_head(i))
		strbuf_puts(sb, "|  ");

	strbuf_puts(sb, bitmap[i] ? "\033[1;33m1\033[0m" : "0");

	if (is_byte_midd(i))
		strbuf_puts(sb, "  |  ");
	else if (!is_byte_tail(i))
		strbuf_puts(sb, "  ");
}

static const char *hex_char_map = "0123456789ABCDEF";

static inline void do_print_hex_dump_color(struct strbuf *sb, char c)
{
	if (c - '0')
		strbuf_printf(sb, "\033[1;33m%c\033[0m", c);
	else
		strbuf_putc(sb, c);
}

/**
 * bitmap must be lsb -> msb
 */
static void do_print_hex_dump(struct strbuf *sb, const u8 *bitmap,
			      size_t i, u8 *tmp)
{
	if (bitmap[i])
		*tmp |= (1 << (i % 8));

	if (is_byte_tail(i)) {
		strbuf_puts(sb, "  |  ");
		do_print_hex_dump_color(sb, hex_char_map[*tmp & 0x0F]);

		strbuf_puts(sb, "  ");

		do_print_hex_dump_color(sb, hex_char_map[(*tmp >> 4) & 0x0F]);
		strbuf_puts(sb, "  |");

		*tmp = 0;
	}
}

void print_frame_dump(const u8 *bitmap, size_t n)
{
	struct strbuf sb;
	strbuf_init(&sb, n);

	size_t i, j = 0;
	u8 tmp = 0;
	for_each_idx(i, n) {
		if (is_byte_head(i))
			strbuf_printf(&sb, "  %" PRIu16 "\t", j);

		do_print_bit_dump(&sb, bitmap, i);
		do_print_hex_dump(&sb, bitmap, i, &tmp);

		if (is_byte_tail(i)) {
			strbuf_printf(&sb, "  %" PRIu16 "\n", j);
			j++;
		}
	}

	fputs(sb.buf, stdout);
	strbuf_free(&sb);
}

// void print_checksum(const u8 *bitmap, size_t n)
// {
// 	size_t i;
// 	u16 sum = 0;
// 	u8 tmp = 0;
// 	for_each_idx(i, n) {
// 		if (bitmap[i])
// 			tmp |= (1 << (i % 8));

// 		if (is_byte_tail(i)) {
// 			sum += tmp;
// 			tmp = 0;
// 		}
// 	}

// 	sum &= 0x00FF;
// 	printf("checksum: 0x%c%c\n",
// 	       hex_char_map[(sum >> 4) & 0x0F],
// 	       hex_char_map[sum & 0x0F]);
// }

void print_task_avail_stack(const char *tag, void *tsk)
{
	info(tag, "available stack size is %" PRIu16 " words\n",
	     uxTaskGetStackHighWaterMark((TaskHandle_t)tsk));
}
