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

#include "power.h"
#include "esp_sleep.h"
#include "jumper.h"
#include "calc.h"
#include "list.h"
#include "termio.h"
#include "driver/rtc_io.h"
#include "soc/gpio_num.h"
#include "wifi.h"
#include "sign.h"

int verify_wakeup_jumper(const u8 *js, size_t j)
{
	size_t i;
	for_each_idx(i, j) {
		if (!esp_sleep_is_valid_wakeup_gpio(js[i]))
			return i;
	}

	return 0;
}

static void config_input_jumper(const u8 *js, size_t n)
{
	size_t i;
	for_each_idx(i, n) {
		rtc_gpio_pullup_dis(js[i]);
		rtc_gpio_pulldown_en(js[i]);
	}
}

static void config_output_jumper(const u8 *js, size_t n)
{
	size_t i;
	for_each_idx(i, n) {
		rtc_gpio_pullup_en(js[i]);
		rtc_gpio_pulldown_dis(js[i]);
	}
}

void setup_external_wakeup(void)
{
	size_t n;
	const u8 *jumpers;

	n = get_output_jumper(&jumpers);
	config_output_jumper(jumpers, n);

	n = get_input_jumper(&jumpers);
	config_input_jumper(jumpers, n);

	u64 mask = get_jumper_input_bitmap() | get_jumper_output_bitmap();
	esp_sleep_enable_ext1_wakeup_io(mask, ESP_EXT1_WAKEUP_ALL_LOW);
}

void setup_timer_wakeup(u64 seconds)
{
	esp_sleep_enable_timer_wakeup(st_mult(seconds, 1000000ULL));
}

void start_deep_sleep(void)
{
	show_sign(SIGN_OFF);
	if (is_sta2ap_connected())
		disconnect_sta2ap();

	esp_deep_sleep_start();
}
