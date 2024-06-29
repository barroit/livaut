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

#ifndef MEMORY_H
#define MEMORY_H

#include "usage.h"
#include <stdlib.h>
#include "type.h"

#define fixed_growth(x) (((x + 16) * 3) / 2)

#define to_boundary_32(x) ((x + 31) & ~31)

static inline void *xmalloc(size_t n)
{
	void *m = malloc(n);
	if (m != NULL)
		return m;

	fatal("malloc() failed (tried to allocate %" PRIu16 " bytes", n);
}

#define xmalloc_b32(n) xmalloc(to_boundary_32(n))

#define bit_sz_to_hex_len(x) (((x / 8) * 2))

void bit_arr_to_hex_str_ll(const u8 *b, size_t bs, char *hb, int msb);

#define bit_arr_to_hex_str_msb(b, bs, hb)\
	bit_arr_to_hex_str_ll(b, bs, hb, 1)

#define bit_arr_to_hex_str_lsb(b, bs, hb)\
	bit_arr_to_hex_str_ll(b, bs, hb, 0)

void bit_arr_to_bit_str(const u8 *b, size_t bs, char *bb);

#endif /* MEMORY_H */
