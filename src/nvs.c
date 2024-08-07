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

#include "nvs.h"
#include "nvs_flash.h"
#include "termio.h"

#define TAG "nvs_flash"

int init_nvs_flash(void)
{
	int err;

	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
	    err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		err = CE(nvs_flash_erase());
		if (err)
			return 1;

		err = nvs_flash_init();
	}

	CE(err);
	if (err)
		return 1;

	info(TAG, "initialized");

	return 0;
}

void release_nvs_flash(void)
{
	CE(nvs_flash_deinit());

	info(TAG, "released");
}
