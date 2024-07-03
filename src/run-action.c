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

#include "run-action.h"
#include "jumper.h"
#include "sign.h"
#include "debug.h"
#include "list.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "termio.h"

static const struct action *find_action(const struct action *acts)
{
	for_each_elem(acts, handle) {
		if (is_jumper_set(J1(acts->jumper), J2(acts->jumper)))
			return acts;
	}

	return NULL;
}

static INTASK void print_available_stack(void)
{
	info("run_action",
	     "available stack size is %" PRIu16 " words\n",
	     uxTaskGetStackHighWaterMark(NULL));
}

static enum action_state next_state(const struct action *acts,
				    const struct action *act,
				    struct action_config *conf,
				    enum action_state state)
{
	switch (state) {
	case ACTION_AGIN:
	case ACTION_RETY:
		return act->handle();
	case ACTION_INIT:
		if (!act)
			return ACTION_INIT;
		else if (act->setup(conf))
			return ACTION_ERRO;

		print_available_stack();
		return act->handle();
	case ACTION_CLEN:
		if (act->teardown())
			return ACTION_ERRO;
		return ACTION_INIT;
	default:
		abort(/* make gcc happy */);
	}
}

static void show_spinner(u8 *sign)
{
	show_sign(*sign);
	*sign ^= 0xFF;
}

static void exec_state(enum action_state state, u8 *sign)
{
	switch (state) {
	case ACTION_DONE:
		show_sign(SN_ON);
		break;
	case ACTION_AGIN:
		show_spinner(sign);
		/* FALLTRHU */
	case ACTION_RETY:
	case ACTION_INIT:
	case ACTION_CLEN:
		break;
	case ACTION_ERRO:
		show_sign(SN_OFF);
		vTaskDelete(NULL);
	}
}

void TASK run_action(void *actions)
{
	u8 sign = SN_1 | SN_3 | SN_5 | SN_7;
	enum action_state state = ACTION_INIT;
	const struct action *acts = actions, *act = NULL;
	struct action_config conf;
	const struct action_config conf0 = {
		.delay = 1000,
	};

	while (39) {
		if (state == ACTION_INIT) {
			show_spinner(&sign);
			act = find_action(acts);
			if (act)
				conf = conf0;
		} else if (find_action(acts) != act) {
			state = ACTION_CLEN;
		} else if (state == ACTION_DONE) {
			show_spinner(&sign);
			state = ACTION_AGIN;
		}

		state = next_state(acts, act, &conf, state);
		exec_state(state, &sign);

		if (state == ACTION_DONE)
			vTaskDelay(pdMS_TO_TICKS(conf0.delay));
		else if (conf.delay)
			vTaskDelay(pdMS_TO_TICKS(conf.delay));
	}
}
