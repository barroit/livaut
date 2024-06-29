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

#ifndef AEHA_PROTOCOL_H
#define AEHA_PROTOCOL_H

#include "driver/rmt_rx.h"
#include "type.h"

enum decoder_state {
	DCD_RTY = -1,
	DCD_NXT,
	DCD_ERR,
};

enum decoder_state decode_aeha_symbols(rmt_symbol_word_t *s, size_t n,
				       u8 **buf, size_t *sz);

void make_aeha_receiver_config(rmt_receive_config_t *cfg);

#endif /* AEHA_PROTOCOL_H */
