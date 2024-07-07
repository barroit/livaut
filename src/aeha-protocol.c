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

#include "aeha-protocol.h"
#include "termio.h"
#include "memory.h"
#include "calc.h"
#include "list.h"
#include <string.h>
#include "esp_attr.h"

#define AEHA_TOLERANCE     150     /* µs */
#define AEHA_MIN_THRESHOLD 1250    /* ns */
#define AEHA_MAX_THRESHOLD 8000000 /* ns */
#define AEHA_TIME_UNIT     440     /* µs */

#define AEHA_T(x) ((x) * AEHA_TIME_UNIT)

#define in_aeha_range(x, t)\
	in_range(x, AEHA_T(t) - AEHA_TOLERANCE, AEHA_T(t) + AEHA_TOLERANCE)

static inline int is_aeha_leader(const rmt_symbol_word_t *sym)
{
	return in_aeha_range(sym->duration0, 8) &&
		in_aeha_range(sym->duration1, 4);
}

static inline int is_aeha_symbols(const rmt_symbol_word_t *syms, size_t n)
{
	return n > 1 && is_aeha_leader(syms);
}

static u8 get_aeha_bit(u16 d1, u16 d2)
{
	u16 derr;

	if (!in_aeha_range(d1, 1))
		derr = d1;
	else if (in_aeha_range(d2, 1))
		return 0;
	else if (in_aeha_range(d2, 3))
		return 1;
	else
		derr = d2;

	error("aeha decoding",
	      "illegal symbol found (duration ‘%" PRIu16 "µs’)", derr);

	return ~0;
}

static enum decoder_state decode_aeha_symbols_step(const rmt_symbol_word_t *s,
						   size_t n, u8 *out)
{
	size_t i;
	u8 bit;

	for_each_idx(i, n) {
		bit = get_aeha_bit(s[i].duration0, s[i].duration1);
		if (bit == (u8)~0)
			return DEC_ERRO;

		*out = bit;
		out++;
	}

	return DEC_DONE;
}

enum decoder_state decode_aeha_symbols(rmt_symbol_word_t *s, size_t n,
				       u8 **out, size_t *sz)
{
	if (!is_aeha_symbols(s, n))
		return DEC_SKIP;

	/* skip leader */
	s++;
	n--;

	if (s[n - 1].duration1 == 0)
		n--;

	if (n % 4 != 0)
		return DEC_SKIP;

	*sz = n;
	*out = xmalloc_b32(n);

	return decode_aeha_symbols_step(s, n, *out);
}

void make_aeha_receiver_config(rmt_receive_config_t *conf)
{
	memset(conf, 0, sizeof(*conf));

	conf->signal_range_min_ns = AEHA_MIN_THRESHOLD;
	conf->signal_range_max_ns = AEHA_MAX_THRESHOLD;
}

enum encoder_state {
	ENCODE_LEADER,
	ENCODE_CUSTOMER,
	ENCODE_DATA,
	ENCODE_TAILER,
};

struct encoder_context {
	rmt_encoder_t base;
	rmt_encoder_t *copy_encoder;
	rmt_encoder_t *byte_encoder;
	rmt_symbol_word_t leading_symbol;
	rmt_symbol_word_t tailing_symbol;
	enum encoder_state state;
	size_t pending;
};

#define encoder_context_of(c) \
	__containerof(c, struct encoder_context, base)

#define handle_encode_result_normal(s, c)	\
	({					\
		if (s & RMT_ENCODING_COMPLETE)	\
			c->state++;		\
		s & RMT_ENCODING_MEM_FULL;	\
	})

static size_t IRAM_ATTR encode_frame(rmt_encoder_t *container,
				     rmt_channel_handle_t channel,
				     const void *data, size_t n,
				     rmt_encode_state_t *state)
{
	struct encoder_context *ctx = encoder_context_of(container);
	rmt_encoder_t *cpenc = ctx->copy_encoder;
	rmt_encoder_t *btenc = ctx->byte_encoder;
	const struct frame_info *frame = data;
	size_t symlen = 0;

	switch (ctx->state) {
	case ENCODE_LEADER:
		symlen += cpenc->encode(cpenc, channel, &ctx->leading_symbol,
					sizeof(rmt_symbol_word_t), state);
		if (handle_encode_result_normal(*state, ctx))
			break;
		/* FALLTHRU */
	case ENCODE_CUSTOMER:
		/**
		 * we just need to manage the state; the copy and byte
		 * encoder handle data truncation recovery
		 */
		symlen += btenc->encode(btenc, channel, frame->data,
					frame->cnum, state);
		if (handle_encode_result_normal(*state, ctx))
			break;
		/* FALLTHRU */
	case ENCODE_DATA:
		symlen += btenc->encode(btenc, channel,
					frame->data + frame->cnum,
					frame->unum, state);
		if (handle_encode_result_normal(*state, ctx))
			break;
		/* FALLTHRU */
	case ENCODE_TAILER:
		symlen += cpenc->encode(cpenc, channel, &ctx->tailing_symbol,
					sizeof(rmt_symbol_word_t), state);
		if (*state & RMT_ENCODING_COMPLETE)
			ctx->state = RMT_ENCODING_RESET;
	}

	return symlen;
}

static int free_encoder(rmt_encoder_t *container)
{
	struct encoder_context *ctx = encoder_context_of(container);

	rmt_del_encoder(ctx->copy_encoder);
	rmt_del_encoder(ctx->byte_encoder);
	free(ctx);

	return 0;
}

static int reset_encoder(rmt_encoder_t *container)
{
	struct encoder_context *ctx = encoder_context_of(container);

	rmt_encoder_reset(ctx->copy_encoder);
	rmt_encoder_reset(ctx->byte_encoder);
	ctx->state = RMT_ENCODING_RESET;

	return 0;
}

#define mae_error(...) error("make_aeha_encoder()", ##__VA_ARGS__)

#define MKSYMB(t0, t1)				\
	{					\
		.level0    = 1,			\
		.duration0 = AEHA_T(t0),	\
		.level1    = 0,			\
		.duration1 = AEHA_T(t1),	\
	}

static int make_encoder_context(struct encoder_context **ctx)
{
	/* rmt_alloc_encoder_mem uses calloc() */
	*ctx = rmt_alloc_encoder_mem(sizeof(struct encoder_context));
	if (!*ctx)
		return ESP_ERR_NO_MEM;

	struct encoder_context *c = *ctx;

	c->base.encode = encode_frame;
	c->base.del    = free_encoder;
	c->base.reset  = reset_encoder;

	c->leading_symbol = (rmt_symbol_word_t)MKSYMB(8, 4);
	c->tailing_symbol = (rmt_symbol_word_t)MKSYMB(1, 0);

	return 0;
}

static int init_copy_encoder(struct encoder_context *ctx)
{
	rmt_copy_encoder_config_t conf; /* fake config */
	return rmt_new_copy_encoder(&conf, &ctx->copy_encoder);
}

static int init_byte_encoder(struct encoder_context *ctx)
{
	rmt_bytes_encoder_config_t conf = {
		.bit0 = MKSYMB(1, 1),
		.bit1 = MKSYMB(1, 3),
	};

	return rmt_new_bytes_encoder(&conf, &ctx->byte_encoder);
}

int make_aeha_encoder(rmt_encoder_handle_t *encoder)
{
	struct encoder_context *ctx;

	int res = make_encoder_context(&ctx);
	if (res)
		goto err_make_enc_ctx;

	res = init_copy_encoder(ctx);
	if (res)
		goto err_init_cp_enc;

	res = init_byte_encoder(ctx);
	if (res)
		goto err_init_bt_enc;

	*encoder = &ctx->base;
	return 0;

err_init_bt_enc:
	rmt_del_encoder(ctx->copy_encoder);
err_init_cp_enc:
	free(ctx);
err_make_enc_ctx:
	return res;
}
