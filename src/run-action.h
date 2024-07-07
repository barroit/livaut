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

#ifndef RUN_ACTION_H
#define RUN_ACTION_H

#include "types.h"

#define ACTION(n, j1, j2)						\
	{ .name = (#n), .handle = (n), .jumper = ((j1 << 8) | j2),	\
	  .setup = (n##_setup), .teardown = (n##_teardown) }

#define ACTION_TAIL() { 0 }

enum action_state {
	ACTION_DONE,
	ACTION_ERRO,
	ACTION_INIT,
	ACTION_AGIN,
	ACTION_RETY,
	ACTION_CLEN,
};

struct action_config {
	u32 delay;
};

typedef enum action_state (*action_handle_t)(void);
typedef int (*action_setup_t)(struct action_config *);
typedef int (*action_teardown_t)(void);

struct action_info {
	const char *name;
	action_handle_t handle;
	action_setup_t setup;
	action_teardown_t teardown;
	u16 jumper;
};

#define J1(j) (j >> 8)
#define J2(j) (j & 0x00FF)

void exec_action_task(void *actions);

#define DEFINE_ACTION_SIGNATURE(n)		\
	enum action_state n(void);		\
	int n##_setup(struct action_config *);	\
	int n##_teardown(void)

#endif /* RUN_ACTION_H */
