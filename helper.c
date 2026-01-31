#include "helper.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

uint8_t file_exists(char *path) {
	FILE *file;
	file = fopen(path, "r");
	if (file == NULL) {
		return 0;
	}
	fclose(file);
	return 1;
}
uint8_t is_valid_src(char *path) {
	uint8_t len = strlen(path);
	if (path[len - 1] == 'c' && path[len - 2] == '.') {
		return 1;
	}
	return 0;
}
