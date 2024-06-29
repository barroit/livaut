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

#ifndef USAGE_H
#define USAGE_H

#include "esp_log.h"
#include "esp_err.h"
#include <stdlib.h>

#define die(t, f, ...)				\
	do {					\
		ESP_LOGE(t, f, ##__VA_ARGS__);	\
		exit(EXIT_FAILURE);		\
	} while (0)
	

#define error(t, f, ...) \
	ESP_LOGE(t, f, ##__VA_ARGS__)

#define warning(t, f, ...) \
	ESP_LOGW(t, f, ##__VA_ARGS__)

#define info(t, f, ...) \
	ESP_LOGI(t, f, ##__VA_ARGS__)

/* report on error (esp family) */
#define ROE_ESP(c, t)						\
	({							\
		esp_err_t r = c;				\
		if (r != ESP_OK)				\
			error(t, "%s", esp_err_to_name(r));	\
		r;						\
	})

#endif
