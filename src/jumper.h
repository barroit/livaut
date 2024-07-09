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

#ifndef JUMPER_H
#define JUMPER_H

#include "types.h"

int config_jumper(void);

int is_jumper_set(u8 j1, u8 j2);

size_t get_output_jumper(const u8 **j);

size_t get_input_jumper(const u8 **j);

u64 get_jumper_output_bitmap(void);

u64 get_jumper_input_bitmap(void);

#endif /* JUMPER_H */
