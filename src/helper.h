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

#ifndef HELPER_H
#define HELPER_H

#include "esp_check.h"

typedef uint8_t u8;
typedef uint16_t u16;

#define FIELD_TYPEOF(t, f) typeof(((t *)0)->f)

#define error(t, f, ...) \
	ESP_LOGE(t, f, ##__VA_ARGS__)

#define warning(t, f, ...) \
	ESP_LOGW(t, f, ##__VA_ARGS__)

/* report on error (esp family) */
#define ROE_ESP(c, t)						\
	({							\
		esp_err_t r = c;				\
		if (r >= 0 && r != ESP_OK)			\
			error(t, "%s", esp_err_to_name(r));	\
		r != ESP_OK;					\
	})

#define for_each_idx(i, n) for (i = 0; i < n; i++)

#endif /* HELPER_H */
