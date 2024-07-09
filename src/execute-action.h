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

#ifndef EXECUTE_ACTION_H
#define EXECUTE_ACTION_H

void execute_action(void *actions);

enum action_result {
	EXEC_DONE,
	EXEC_ERROR,
	EXEC_AGAIN,
	EXEC_RETRY,
};

typedef enum action_result (*action_handle_t)(void);
typedef int (*action_setup_t)(void);
typedef int (*action_teardown_t)(void);

struct action {
	const char *name;
	action_handle_t handle;
	action_setup_t setup;
	action_teardown_t teardown;
	unsigned jprsrc;
	unsigned jprdest;
};

#define ACTION_DECLARATION(n)		\
	enum action_result n(void);	\
	int n##_setup(void);		\
	int n##_teardown(void)

#define ACT(n, j1, j2)				\
	{					\
		.name     = (#n),		\
		.handle   = (n),		\
		.jprsrc   = (j1),		\
		.jprdest  = (j2),		\
		.setup    = (n##_setup),	\
		.teardown = (n##_teardown),	\
	}

#define ACT_END() { 0 }

#endif /* EXECUTE_ACTION_H */
