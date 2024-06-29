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

#include "memory.h"
#include "list.h"

static const char *hex_char_map = "0123456789ABCDEF";

static inline void byte_to_hex(uint8_t b, char *hb)
{
	hb[0] = hex_char_map[(b >> 4) & 0x0F];
	hb[1] = hex_char_map[b & 0x0F];
}

void bit_arr_to_hex_str_ll(const u8 *b, size_t bn, char *h, int msb)
{
	size_t i, j = 0;
	u8 tmp = 0;

	for_each_idx(i, bn) {
		if (b[i] == 1)
			tmp |= (1 << (msb ? (7 - i % 8) : (i % 8)));

		if ((i % 8 == 7) || i == bn - 1) {
			byte_to_hex(tmp, h + j);
			j += 2;
			tmp = 0;
		}
	}

	h[j] = 0;
}

void bit_arr_to_bit_str(const u8 *b, size_t bs, char *bb)
{
	size_t i;

	for_each_idx(i, bs)
		bb[i] = b[i] + '0';

	bb[i] = 0;
}
