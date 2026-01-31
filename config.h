#pragma once
#include <stdint.h>
#include "constants.h"

enum COMP_FLAG {
	DEBUG,
	RELEASE
};

typedef struct {
	char *array;
	uint16_t len;
	uint16_t size;
} src_t;

typedef struct {
	char *src_array;
	uint16_t src_len;

	uint8_t *on_array;
	uint16_t on_len;

	char out[MAX_TOK_LEN];
} config_t;

void src_init(src_t *src);
void src_add(src_t *src, char *path);
