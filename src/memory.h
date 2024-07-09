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

#include "termio.h"
#include <stdlib.h>
#include "types.h"
#include "calc.h"

static inline void *xmalloc(size_t n)
{
	void *buf = malloc(n);
	if (buf != NULL)
		return buf;

	die("malloc()",
	    "out of memory (tried to allocate %" PRIu16 " bytes", n);
}

static inline void *xrealloc(void *p, size_t n)
{
	void *buf = realloc(p, n);
	if (buf != NULL)
		return buf;

	die("realloc()",
	    "out of memory (tried to allocate %" PRIu16 " bytes", n);
}

#define xmalloc_b32(n) xmalloc(to_boundary_32(n))

#define xrealloc_b32(p, n) xrealloc(p, to_boundary_32(n))

#define REALLOC_ARRAY(p, n) xrealloc_b32(p, st_mult(sizeof(*(p)), n))

#define CAPACITY_GROW(ptr, len, cap)				\
	do {							\
		if (len > cap) {				\
			cap = fixed_grow(cap);			\
			cap = cap < (len) ? (len) : cap;	\
			ptr = REALLOC_ARRAY(ptr, cap);		\
		}						\
	} while (0)

#define sizeof_array(a) (sizeof(a) / sizeof(*(a)))

#endif /* MEMORY_H */
