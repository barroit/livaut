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
#include "list.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "types.h"
#include "termio.h"
#include "calc.h"
#include "power.h"
#include "memory.h"

#define TAG "jumper"

static const u8 output_jumpers[] = {
	CONFIG_JUMPER_OUTPUT,
};

static const u8 input_jumpers[]  = {
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
		bitmap |= gpio_bit_mask(js[i]);

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

size_t get_output_jumper(const u8 **j)
{
	*j = output_jumpers;
	return sizeof_array(output_jumpers);
}

size_t get_input_jumper(const u8 **j)
{
	*j = input_jumpers;
	return sizeof_array(input_jumpers);
}

u64 get_jumper_output_bitmap(void)
{
	return get_pin_bitmap(output_jumpers, sizeof_array(output_jumpers));
}

u64 get_jumper_input_bitmap(void)
{
	return get_pin_bitmap(input_jumpers, sizeof_array(input_jumpers));
}

int config_jumper(void)
{
	int err;

	err = verify_wakeup_jumper(output_jumpers,
				   sizeof_array(output_jumpers));
	if (err)
		goto err_wkup_jmpr;

	err = verify_wakeup_jumper(input_jumpers,
				   sizeof_array(input_jumpers));
	if (err)
		goto err_wkup_jmpr;

	u64 outmask = get_jumper_output_bitmap();
	gpio_config_t out_conf = get_out_conf(outmask);
	gpio_config(&out_conf);

	u64 inmask = get_jumper_input_bitmap();
	gpio_config_t in_conf = get_in_conf(inmask);
	gpio_config(&in_conf);

	return 0;

err_wkup_jmpr:
	error(TAG, "‘%d’ is not a valid rtc gpio", err);
	return 1;
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
	int pass;

	pass = is_valid_jumper(output_jumpers,
			       sizeof_array(output_jumpers), j1);
	if (!pass)
		die(TAG, "j1 %" PRIu8 " is not an output jumper", j1);

	pass = is_valid_jumper(input_jumpers,
			       sizeof_array(input_jumpers), j2);
	if (!pass)
		die(TAG, "j2 %" PRIu8 " is not an input jumper", j2);

	return is_gpio_connected(j1, j2, 1) && is_gpio_connected(j1, j2, 0);
}
