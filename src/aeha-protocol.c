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
#include "termio.h"
#include "memory.h"
#include "calc.h"
#include "list.h"
#include <string.h>

#define AEHA_TOLERANCE     150     /* µs */
#define AEHA_MIN_THRESHOLD 1250    /* ns */
#define AEHA_MAX_THRESHOLD 8000000 /* ns */
#define AEHA_TIME_UNIT     440     /* µs */
#define AEHA_DATA_OFFSET   7

#define AEHA_T(x) ((x) * AEHA_TIME_UNIT)

#define in_aeha_range(x, t)\
	in_range(x, AEHA_T(t) - AEHA_TOLERANCE, AEHA_T(t) + AEHA_TOLERANCE)

static inline int is_aeha_leader(const rmt_symbol_word_t *sym)
{
	return in_aeha_range(sym->duration0, 8) &&
		in_aeha_range(sym->duration1, 4);
}

static inline int is_aeha_symbols(const rmt_symbol_word_t *syms, size_t n)
{
	return n > 1 && is_aeha_leader(syms);
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

	error("aeha decoding",
	      "illegal symbol found (duration ‘%" PRIu16 "µs’)", derr);

	return ~0;
}

static enum decoder_state decode_aeha_symbols_step(const rmt_symbol_word_t *s,
						   size_t n, u8 *out)
{
	size_t i;
	u8 bit;

	for_each_idx(i, n) {
		bit = get_aeha_bit(s[i].duration0, s[i].duration1);
		if (bit == (u8)~0)
			return DEC_ERRO;

		*out = bit;
		out++;
	}

	return DEC_DONE;
}

enum decoder_state decode_aeha_symbols(rmt_symbol_word_t *s, size_t n,
				       u8 **out, size_t *sz)
{
	if (!is_aeha_symbols(s, n))
		return DEC_SKIP;

	/* skip leader */
	s++;
	n--;

	if (s[n - 1].duration1 == 0)
		n--;

	if (n % 4 != 0)
		return DEC_SKIP;

	*sz = n;
	*out = xmalloc_b32(n);

	return decode_aeha_symbols_step(s, n, *out);
}

void make_aeha_receiver_config(rmt_receive_config_t *conf)
{
	memset(conf, 0, sizeof(*conf));

	conf->signal_range_min_ns = AEHA_MIN_THRESHOLD;
	conf->signal_range_max_ns = AEHA_MAX_THRESHOLD;
}
