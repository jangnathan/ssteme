#pragma once
#include <stdint.h>
#include "constants.h"
#include "config.h"

typedef struct {
	char out[MAX_TOK_LEN];
	src_t src;
} parsed_t;

uint8_t parse_file(parsed_t *parse);
void parse_init(parsed_t *parse);
