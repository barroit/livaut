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

#ifndef STRBUF_H
#define STRBUF_H

#include "types.h"
#include <stddef.h>

struct strbuf {
	size_t len;
	size_t cap;
	char *buf;
};

void strbuf_init(struct strbuf *sb, size_t n);

size_t strbuf_putc(struct strbuf *sb, char c);

size_t strbuf_puts(struct strbuf *sb, const char *str);

size_t strbuf_printf(struct strbuf *sb, const char *fmt, ...);

void strbuf_free(struct strbuf *sb);

#endif /* STRBUF_H */
