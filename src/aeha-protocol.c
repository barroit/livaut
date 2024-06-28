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

#include "aeha-protocol.h"
#include "helper.h"
#include <string.h>
#include <stdbool.h>

#define AEHA_TOLERANCE     150 /* µm */
#define AEHA_MIN_THRESHOLD 1250    /* nm */
#define AEHA_MAX_THRESHOLD 8000000 /* nm */
#define AEHA_TIME_UNIT     440     /* µm */
#define AEHA_DATA_OFFSET   7

#define AEHA_T(x) ((x) * AEHA_TIME_UNIT)

#define in_aeha_range(x, t)\
	in_range(x, AEHA_T(t) - AEHA_TOLERANCE, AEHA_T(t) + AEHA_TOLERANCE)

static inline bool is_aeha_frame(size_t n)
{
	return n == 162 || n == 154 || n == 6;
}

static inline bool is_aeha_leader(const rmt_symbol_word_t *sym)
{
	return in_aeha_range(sym->duration0, 8) &&
		in_aeha_range(sym->duration1, 4);
}

static inline bool is_aeha_symbols(const rmt_symbol_word_t *syms, size_t n)
{
	return is_aeha_frame(n) && is_aeha_leader(syms);
}

static u8 get_aeha_bit(u16 d1, u16 d2)
{
	u16 derr;

	if (!in_aeha_range(d1, 1))
		derr = d1;
	else if (in_aeha_range(d2, 1))
		return 0;
	else if (in_aeha_range(d2, 3))
		return 1;
	else
		derr = d2;

	ESP_LOGE("aeha decoding",
		 "illegal symbol found (duration ‘%" PRIu16 "µm’)", derr);

	return ~0;
}

static enum decoder_state decode_aeha_symbols_step(
	const rmt_symbol_word_t *syms,
	size_t n,
	u8 *buf)
{
	size_t i;
	u8 bit;

	for_each_idx(i, n) {
		bit = get_aeha_bit(syms[i].duration0, syms[i].duration1);
		if (bit == (u8)~0)
			return DCD_ERR;

		*buf = bit;
		buf++;
	}

	return DCD_NXT;
}

enum decoder_state decode_aeha_symbols(rmt_symbol_word_t *syms, size_t n,
				       u8 *buf, size_t *sz)
{
	if (!is_aeha_symbols(syms, n))
		return DCD_RTY;

	/* skip leader */
	syms++;
	n--;

	/**
	 * duration1 of the last symbol is 0, we simply skip this symbol
	 */
	n--;

	*sz = n;
	return decode_aeha_symbols_step(syms, n, buf);
}

void make_aeha_receiver_config(rmt_receive_config_t *cfg)
{
	memset(cfg, 0, sizeof(*cfg));

	cfg->signal_range_min_ns = AEHA_MIN_THRESHOLD;
	cfg->signal_range_max_ns = AEHA_MAX_THRESHOLD;
}
