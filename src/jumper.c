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

#include "jumper.h"
#include "jumper-layout.h"
#include "list.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "types.h"
#include "termio.h"
#include "calc.h"

#define TAG "jumper"

static const u8 output_jumper[] = {
	CONFIG_JUMPER_OUTPUT,
};

static const u8 input_jumper[]  = {
	CONFIG_JUMPER_INPUT_1,
	CONFIG_JUMPER_INPUT_2,
	CONFIG_JUMPER_INPUT_3,
	CONFIG_JUMPER_INPUT_4,
	CONFIG_JUMPER_INPUT_5
};

static u64 get_pin_bitmap(const u8 *js, size_t n)
{
	size_t i;
	u64 bitmap = 0;

	for_each_idx(i, n)
		bitmap |= pin_bit_mask(js[i]);

	return bitmap;
}

static gpio_config_t get_out_conf(u64 pin)
{
	gpio_config_t conf = {
		.intr_type    = GPIO_INTR_DISABLE,
		.mode         = GPIO_MODE_OUTPUT,
		.pull_up_en   = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pin_bit_mask = pin,
	};

	return conf;
}

static gpio_config_t get_in_conf(u64 pin)
{
	gpio_config_t conf = {
		.intr_type    = GPIO_INTR_DISABLE,
		.mode         = GPIO_MODE_INPUT,
		.pull_up_en   = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pin_bit_mask = pin,
	};

	return conf;
}

void config_jumper(void)
{
	u64 output = get_pin_bitmap(output_jumper, sizeof(output_jumper));
	u64 input = get_pin_bitmap(input_jumper, sizeof(input_jumper));

	gpio_config_t out_conf = get_out_conf(output);
	gpio_config(&out_conf);

	gpio_config_t in_conf = get_in_conf(input);
	gpio_config(&in_conf);
}

static int is_valid_jumper(const u8 *js, size_t n, u8 j)
{
	size_t i;
	for_each_idx(i, n) {
		if (js[i] == j)
			return 1;
	}
	return 0;
}

static int is_gpio_connected(u8 j1, u8 j2, u32 lv)
{
	gpio_set_level(j1, lv);
	return gpio_get_level(j2) == lv;
}

int is_jumper_set(u8 j1, u8 j2)
{
	if (!is_valid_jumper(output_jumper, sizeof(output_jumper), j1))
		die(TAG, "j1 %" PRIu8 " is not a output jumper", j1);
	else if (!is_valid_jumper(input_jumper, sizeof(input_jumper), j2))
		die(TAG, "j2 %" PRIu8 " is not a input jumper", j2);

	return is_gpio_connected(j1, j2, 1) && is_gpio_connected(j1, j2, 0);
}
