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

#ifndef CALC_H
#define CALC_H

#include "termio.h"
#include <limits.h>

#define in_range(c, a, b) (((a) < (c)) && ((b) > (c)))

#define fixed_grow(a) ((((a) + 16) * 3) / 2)

#define to_boundary_32(a) (((a) + 3) & ~3)

#define bitsizeof(a) (CHAR_BIT * sizeof(a))

#define max_uint_val(a) (UINTMAX_MAX >> (bitsizeof(uintmax_t) - bitsizeof(a)))

#define uint_mult_overflows(a, b) ((a) && ((b) > (max_uint_val(a) / (a))))

static inline size_t st_mult(size_t a, size_t b)
{
	if (!uint_mult_overflows(a, b))
		return a * b;

	die("st_mult()", "size overflow (%" PRIuMAX " * %" PRIuMAX ")",
	    (uintmax_t)a, (uintmax_t)b);
}

#endif /* CALC_H */
