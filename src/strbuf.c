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

#include "strbuf.h"
#include "memory.h"
#include <string.h>
#include <stdarg.h>

static inline int is_cap_avail(struct strbuf *sb, size_t len)
{
	return sb->len + len + 1 < sb->cap;
}

static inline void strbuf_grow(struct strbuf *sb, size_t len)
{
	CAPACITY_GROW(sb->buf, sb->len + len + 1, sb->cap);
}

void strbuf_init(struct strbuf *sb, size_t n)
{
	memset(sb, 0, sizeof(*sb));
	strbuf_grow(sb, n);
}

size_t strbuf_putc(struct strbuf *sb, char c)
{
	if (!is_cap_avail(sb, 1))
		strbuf_grow(sb, 1);

	sb->buf[sb->len++] = c;
	sb->buf[sb->len] = 0;

	return 1;
}

size_t strbuf_puts(struct strbuf *sb, const char *str)
{
	size_t n = strlen(str);

	if (!is_cap_avail(sb, n))
		strbuf_grow(sb, n);

	memcpy(sb->buf + sb->len, str, n + 1);
	sb->len += n;

	return n;
}

static size_t strbuf_vprintf(struct strbuf *sb, const char *fmt, va_list ap)
{
	va_list cp;
	va_copy(cp, ap);

	int n = vsnprintf(sb->buf + sb->len, sb->cap - sb->len, fmt, cp);
	if (n < 0)
		die("vsnprintf()", "function broken (returned %d)", n);
	va_end(cp);

	if (!is_cap_avail(sb, (size_t)n)) {
		strbuf_grow(sb, (size_t)n);
		n = vsnprintf(sb->buf + sb->len, sb->cap - sb->len, fmt, ap);
	}

	sb->len += n;
	return n;
}

size_t strbuf_printf(struct strbuf *sb, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	size_t n = strbuf_vprintf(sb, fmt, ap);

	va_end(ap);
	return n;
}

void strbuf_free(struct strbuf *sb)
{
	free(sb->buf);
}
