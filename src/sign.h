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

#ifndef SIGN_H
#define SIGN_H

#define SN_1 (1 << 0)
#define SN_2 (1 << 1)
#define SN_3 (1 << 2)
#define SN_4 (1 << 3)
#define SN_5 (1 << 4)
#define SN_6 (1 << 5)
#define SN_7 (1 << 6)
#define SN_8 (1 << 7)
#define SN_ON (~0)
#define SN_OF (0)

esp_err_t init_sign(void);

/**
 * plot sign, this function can be used anywhere
 */
esp_err_t show_sign(u8 code);

#endif /* SIGN_H */
