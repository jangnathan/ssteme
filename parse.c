#include "parse.h"
#include "helper.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

enum TOK_STATE {
	ST_NONE,
	ST_LEX,
	ST_STRING,
	ST_END,
};

enum VAR_TYPE {
	VAR_ON,
	VAR_SRC,
	VAR_OUT
};

enum TOK_TYPE {
	TY_LEX,
	TY_STRING
};

typedef struct {
	parsed_t *parsed;
	char tok[MAX_TOK_LEN];
	enum TOK_STATE tok_state;
	enum VAR_TYPE temp_var_enum;
	enum TOK_TYPE temp_type;
	uint8_t tok_idx;
	uint8_t is_setting;
	uint8_t can_end;

	uint16_t char_idx;
} parse_ctx_t;

uint8_t is_whitespace(char ch) {
	if (ch == ' ' || ch == '\t' || ch == '\n') return 1;
	return 0;
}

uint8_t is_lex(char ch) {
	uint8_t chn = (uint8_t)ch;
	if (chn >= 65 && chn <= 90) return 1;
	if (chn >= 97 && chn <= 122) return 1;
	if (chn >= 48 && chn <= 57) return 1;
	return 0;
}

uint8_t parse_tok(parse_ctx_t *ctx, char ch) {
	switch (ctx->tok_state) {
		case ST_NONE:
			if (is_lex(ch)) {
				ctx->tok_state = ST_LEX;
				ctx->temp_type = TY_LEX;

				ctx->tok[0] = ch;
				ctx->tok_idx = 1;
				break;
			}
			if (ch == '"') {
				ctx->tok_idx = 0;
				ctx->tok_state = ST_STRING;
				ctx->temp_type = TY_STRING;
				break;
			}
			if (!is_whitespace(ch)) {
				fprintf(stderr, "unexpected character'%c'", ch);
				return 0;
			}
		case ST_LEX: {
			if (is_lex(ch)) {
				ctx->tok[ctx->tok_idx] = ch;
				ctx->tok_idx++;
				break;
			}
			if (is_whitespace(ch)) {
				ctx->tok[ctx->tok_idx] = '\0';
				ctx->tok_state = ST_END;

				ctx->can_end = 1;
				break;
			}
			if (ch == ',') {
				ctx->can_end = 1;
				break;
			}
			fprintf(stderr, "unexpected character'%c'", ch);
			return 0;
		}
		case ST_STRING:
			if (ch == '"') {
				ctx->tok_state = ST_END;
				ctx->tok[ctx->tok_idx] = '\0';
				ctx->tok_idx = 0;
				break;
			}
			ctx->tok[ctx->tok_idx] = ch;
			ctx->tok_idx++;
			break;
		case ST_END:
			ctx->can_end = 1;
			if (is_lex(ch)) {
				ctx->is_setting = 0;
				break;
			}
			if (is_whitespace(ch)) break;
			if (ch == ',' || ch == '\0') break;

			fprintf(stderr, "unexpected character'%c' %d", ch, ctx->char_idx);
			return 0;
	}
	return 1;
}

uint8_t parse_is_setting(parse_ctx_t *ctx, char ch) {
	uint8_t result = parse_tok(ctx, ch);
	if (result == 0) return 0;
	switch (ctx->temp_var_enum) {
		case VAR_SRC:
			if (ctx->can_end) {
				if (ch == ',' || ch == '\0' || is_lex(ch)) {
					if (ctx->temp_type != TY_STRING) {
						fprintf(stderr, "has to be a string");
						return 0;
					}
					uint8_t len = strlen(ctx->tok);
					if (!is_valid_src(ctx->tok) && ctx->tok[len] == '/') {
						fprintf(stderr, "%s must end in .c", ctx->tok);
						return 0;
					}
					ctx->tok_state = ST_NONE;
					ctx->can_end = 0;

					src_add(&ctx->parsed->src, ctx->tok);

					if (is_lex(ch)) {
						ctx->tok[0] = ch;
						ctx->tok_idx = 1;
					}
				}
			}
			break;
		case VAR_ON: {
			break;
		}
		case VAR_OUT: {
			if (ctx->can_end) {
				if (ch == '\0' || is_lex(ch)) {
					ctx->tok_state = ST_NONE;
					ctx->can_end = 0;

					strcpy(ctx->parsed->out, ctx->tok);
					break;
				}
				if (is_whitespace(ch)) break;
				fprintf(stderr, "whyunexpected char '%c'", ch);
				return 0;
			}
			break;
		}
	}
	return 1;
}

uint8_t parse_not_setting(parse_ctx_t *ctx, char ch) {
	switch (ctx->tok_state) {
		case ST_NONE: {
			if (is_lex(ch)) {
				ctx->tok_state = ST_LEX;
				ctx->tok[ctx->tok_idx] = ch;
				ctx->tok_idx++;
				break;
			}
			if (!is_whitespace(ch)) {
				fprintf(stderr, "unexpected character '%c'", ch);
				return 0;
			}
			break;
		}
		case ST_LEX: {
			if (is_lex(ch)) {
				ctx->tok[ctx->tok_idx] = ch;
				ctx->tok_idx++;
				break;
			}

			if (is_whitespace(ch)) {
				ctx->tok_state = ST_END;
				break;
			}
			if (ch == '=') {
				goto tok_end;
			}
			fprintf(stderr, "2unexpected character '%c'", ch);
			return 0;
		}
		case ST_END: {
			if (ch == '=') {
			tok_end:
				ctx->tok[ctx->tok_idx] = '\0';
				ctx->tok_state = ST_NONE;
				ctx->is_setting = 1;
				if (strcmp(ctx->tok, "src") == 0) {
					ctx->temp_var_enum = VAR_SRC;
					break;
				}
				if (strcmp(ctx->tok, "out") == 0) {
					ctx->temp_var_enum = VAR_OUT;
					break;
				}
				break;
			}
			if (!is_whitespace(ch)) {
				fprintf(stderr, "1unexpected character '%c'", ch);
				return 0;
			}
			break;
		}
		default: {
			fprintf(stderr, "error");
			return 0;
		}
	}
	return 1;
}

uint8_t parse(parse_ctx_t *ctx, char ch) {
	if (ctx->is_setting) {
		uint8_t result = parse_is_setting(ctx, ch);
		if (result == 0) return 0;
	} else {
		uint8_t result = parse_not_setting(ctx, ch);
		if (result == 0) return 0;
	}
	return 1;
}

uint8_t parse_file(parsed_t *parsed) {
	FILE *file;
	file = fopen(CONFIG_NAME, "r");

	if (file == NULL) {
		fprintf(stderr, "cannot find parsed file in this directory");
		return 0;
	}

	parse_ctx_t ctx;
	ctx.parsed = parsed;
	ctx.temp_var_enum = VAR_ON;
	ctx.tok_state = ST_NONE;
	ctx.tok_idx = 0;
	ctx.is_setting = 0;
	ctx.can_end = 0;
	ctx.char_idx = 0;

	char ch;
	while ((ch = fgetc(file)) != EOF) {
		ctx.char_idx++;
		if(parse(&ctx, ch) == 0) return 0;
	}
	if(parse(&ctx, '\0') == 0) return 0;

	fclose(file);
	return 1;
}

void parse_init(parsed_t *parse) {
	parse->out[0] = '\0';
	src_init(&parse->src);
}
