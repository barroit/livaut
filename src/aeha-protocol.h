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
#include "driver/rmt_tx.h"
#include "types.h"
#include "signal-schedule-def.h"

enum decoder_state {
	DEC_DONE,
	DEC_SKIP,
	DEC_ERRO,
};

enum decoder_state decode_aeha_symbols(rmt_symbol_word_t *s, size_t n,
				       u8 **buf, size_t *sz);

void make_aeha_receiver_config(rmt_receive_config_t *conf);

#define AEHA_DUTY_CYCLE 0.33
#define AEHA_FREQUENCY  38000 /* hz */

int make_aeha_encoder(rmt_encoder_handle_t *encoder);

int convert_aeha_lldat(u32 *space, size_t n);

int is_lldat_converted(u32 *space);

#endif /* AEHA_PROTOCOL_H */
