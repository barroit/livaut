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

#ifndef TERMIO_H
#define TERMIO_H

#include "types.h"
#include "esp_log.h"
#include "esp_err.h"
#include <stdlib.h>

void print_frame_dump(const u8 *bitmap, size_t n);

#define die(t, f, ...)				\
	do {					\
		ESP_LOGE(t, f, ##__VA_ARGS__);	\
		abort();			\
	} while (0)

#define error(t, f, ...)			\
	({					\
		ESP_LOGE(t, f, ##__VA_ARGS__);	\
		1;				\
	})

#define warning(t, f, ...)			\
	({					\
		ESP_LOGW(t, f, ##__VA_ARGS__);	\
		1;				\
	})

#define info(t, f, ...) \
	ESP_LOGI(t, f, ##__VA_ARGS__)

void print_task_avail_stack(const char *tag, void *tsk);

#define HH_MM_SS "%02" PRIu64 ":%02" PRIu64 ":%02" PRIu64

#define CE ESP_ERROR_CHECK_WITHOUT_ABORT

#endif /* TERMIO_H */
