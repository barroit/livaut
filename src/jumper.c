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

static const u8 jumper_output[] = { JUMPER_HIGH };
static const u8 jumper_input[]  = { JUMPER_LOW };

static u64 get_pin_bitmap(const u8 *pins, size_t n)
{
	size_t i;
	u64 bitmap = 0;

	for_each_idx(i, n)
		bitmap |= pin_bit_mask(pins[i]);

	return bitmap;
}

static void config_jumper_output_gpio(u64 pins)
{
	gpio_config_t conf = {
		.intr_type    = GPIO_INTR_DISABLE,
		.mode         = GPIO_MODE_OUTPUT,
		.pull_up_en   = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pin_bit_mask = pins,
	};

	ESP_ERROR_CHECK(gpio_config(&conf));
}

static void config_jumper_input_gpio(u64 pins)
{
	gpio_config_t conf = {
		.intr_type    = GPIO_INTR_DISABLE,
		.mode         = GPIO_MODE_INPUT,
		.pull_up_en   = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pin_bit_mask = pins,
	};

	ESP_ERROR_CHECK(gpio_config(&conf));
}

void config_jumper(void)
{
	u64 out = get_pin_bitmap(jumper_output, sizeof(jumper_output)),
	    in = get_pin_bitmap(jumper_input, sizeof(jumper_input));

	config_jumper_output_gpio(out);
	config_jumper_input_gpio(in);
}

static int is_input_gpio(u8 j)
{
	size_t i;

	for_each_idx(i, sizeof(jumper_input))
		if (jumper_input[i] == j)
			return 1;

	return 0;
}

static int is_gpio_connected(u8 j1, u8 j2, u32 lv)
{
	gpio_set_level(j1, lv);
	return gpio_get_level(j2) == lv;
}

int is_jumper_set(u8 j1, u8 j2)
{
	if (!is_input_gpio(j2))
		die("is_jumper_set()", "j2 %" PRIu8 " is not a input gpio",
		    j2);

	if (!is_gpio_connected(j1, j2, 1))
		return 0;

	if (!is_gpio_connected(j1, j2, 0))
		return 0;

	return 1;
}
