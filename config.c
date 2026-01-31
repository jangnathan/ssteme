#include "config.h"
#include "constants.h"

#include <stdlib.h>
#include <string.h>

// these are path arrays when a user decides to put a directory in src
void src_init(src_t *src) {
	src->size = 16;
	src->len = 0;
	src->array = malloc(src->size * MAX_PATH_LEN);
}
void src_add(src_t *src, char *path) {
	strcpy(src->array + src->len * MAX_PATH_LEN, path);
	src->len++;
	if (src->len >= src->size) {
		src->size *= 2;
		src->array = realloc(src->array, src->size * MAX_PATH_LEN);
	}
}
