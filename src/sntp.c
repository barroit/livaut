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

#include "sntp.h"
#include "wifi.h"
#include "esp_netif_sntp.h"
#include "termio.h"
#include "time.h"

#define TAG "sntp_service"

#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE   "GMT-9"

static int is_service_started;

int setup_sntp_service(void)
{
	if (!is_wifi_connected())
		return 1;

	esp_sntp_config_t conf = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
	conf.start = false;

	esp_netif_sntp_init(&conf);
	return 0;
}

int start_sntp_service(void)
{
	int err;

	esp_netif_sntp_start();

	err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000));
	if (err) {
		warning(TAG,
			"failed to synchronize system time from %s",
			NTP_SERVER);
		return 1;
	}

	is_service_started = 1;
	info(TAG, "synchronized system time");

	time_t now = time(NULL);
	info(TAG, "local time is %s", asctime(localtime(&now)));

	return 0;
}

void collaborate_timezone(void)
{
	setenv("TZ", TIMEZONE, 1);
	tzset();
}

u64 get_seconds_of_day(void)
{
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	return (t->tm_hour * 3600) + (t->tm_min * 60) + t->tm_sec;
}

int is_sntp_started(void)
{
	return is_service_started;
}
