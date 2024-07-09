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

#include "execute-action.h"
#include <stdlib.h>
#include "jumper.h"
#include "list.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sign.h"
#include "termio.h"
#include "power.h"

enum executor_state {
	EXECUTOR_INIT,
	EXECUTOR_SETUP,
	EXECUTOR_EXECUTE,
	EXECUTOR_TEARDOWN,
};

struct executor_context {
	enum executor_state state;
	const struct action *act;
	u8 sign;
	unsigned idles;
};

static const struct action *get_active_action(const struct action *act)
{
	for_each_elem(act, name) {
		if (is_jumper_set(act->jprsrc, act->jprdest))
			return act;
	}

	return NULL;
}

static void run_spinner(u8 *sign)
{
	show_sign(*sign);
	inverse_sign(sign);
}

static int do_execute_action(const struct action *actions,
			     struct executor_context *ctx)
{
	int err;
	enum action_result spec;
	if (ctx->act && !is_jumper_set(ctx->act->jprsrc, ctx->act->jprdest))
		ctx->state = EXECUTOR_TEARDOWN;

	switch (ctx->state) {
	case EXECUTOR_INIT:
		ctx->act = get_active_action(actions);
		if (!ctx->act) {
			run_spinner(&ctx->sign);
			ctx->idles++;
			break;
		}
		ctx->state++;
		/* FALLTHRU */
	case EXECUTOR_SETUP:
		err = ctx->act->setup();
		if (err)
			return 1;
		ctx->state++;
		info(ctx->act->name, "configured");
		/* FALLTHRU */
	case EXECUTOR_EXECUTE:
		spec = ctx->act->handle();
		switch (spec) {
		case EXEC_ERROR:
			return 1;
		case EXEC_AGAIN:
			run_spinner(&ctx->sign);
			goto prepare_next;
		case EXEC_RETRY:
			return 0;
		case EXEC_DONE:
			show_sign(SIGN_ON);
		}
		ctx->state++;
		/* FALLTHRU */
	case EXECUTOR_TEARDOWN:
		err = ctx->act->teardown();
		if (err)
			return 1;
		info(ctx->act->name, "cleaned");

		ctx->act = NULL;
		ctx->state = EXECUTOR_INIT;
	}

prepare_next:
	vTaskDelay(pdMS_TO_TICKS(1500));
	return 0;
}

void execute_action(void *actions)
{
	struct executor_context ctx = {
		.sign = SIGN_1 | SIGN_3 | SIGN_5 | SIGN_7,
	};

	while (39) {
		if (do_execute_action(actions, &ctx))
			start_deep_sleep();

		if (ctx.idles > CONFIG_EXECUTOR_IDLE_TIME) {
			setup_external_wakeup();
			start_deep_sleep();
		}
	}
}
