#include "cache.h"
#include "constants.h"
#include "helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

uint8_t recurse_find(char *path, src_t *src) {
	DIR *dir = opendir(path);
	
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		const char *name = ent->d_name;

		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
			continue;

		char full[4096];
		snprintf(full, sizeof(full), "%s%s", path, name);

		struct stat st;
		if (lstat(full, &st) == -1) {
			fprintf(stderr, "lstat(%s): %s\n", full, strerror(errno));
			continue;
		}

		if (S_ISDIR(st.st_mode)) {
			snprintf(full, sizeof(full), "%s/", full);
			return recurse_find(full, src);
		} else {
			if (!is_valid_src(full)) {
				fprintf(stderr, "%s must end in .c", full);
				return 0;
			}
			uint8_t len = strlen(full);
			full[len - 2] = '\0';
			src_add(src, full);
		}
	}

	closedir(dir);
	return 1;
}

// FUTURE: CAN YOU CHECK IF FILE EXISTS?

uint8_t cache_parsed(parsed_t *parsed) {
	FILE *file;
	file = fopen(CACHE_PATH, "w");

	if (parsed->src.len > 0) {
		src_t src;
		src_init(&src);

		uint8_t i = 0;
		while (1) {
			if (parsed->src.array[i * MAX_PATH_LEN] == '\0') {
				break;
			}

			char *path = parsed->src.array + i * MAX_PATH_LEN;
			uint8_t str_len = strlen(path);
			if (path[str_len-1] == '/') {
				DIR *dir = opendir(path);
				if (!dir) {
					fprintf(stderr, "opendir(%s): %s\n", path, strerror(errno));
					return 0;
				}
				uint8_t result = recurse_find(path, &src);
				if (!result) return 0;
			} else {
				if (!file_exists(path)) return 0;
				path[str_len - 2] = '\0';
				src_add(&src, path);
			}

			i++;
		}

		uint8_t top_byte = src.len >> 8;
		uint8_t bottom_byte = src.len & 0xFF;

		fprintf(file, "s%c%c", (char)top_byte, (char)bottom_byte);
		for (i = 0; i < src.len; i++) {
			fprintf(file, "%s\n", src.array + i * MAX_PATH_LEN);
		}
		fprintf(file, "\1");
	}

	if (parsed->out[0] != '\0') {
		fprintf(file, "o");
		fprintf(file, "%s", parsed->out);
		fprintf(file, "\1");
	}

	return 1;
}

enum c_status {
	C_NONE,
	C_SIZE1,
	C_SIZE2,
	C_STUFF
};
enum c_setting {
	S_SRC,
	S_ON,
	S_OUT
};

uint8_t cache2config(config_t *config) {
	FILE *file;
	file = fopen(CACHE_PATH, "r");

	enum c_status status = C_NONE;
	enum c_setting setting;
	uint16_t temp_size = 0;

	uint8_t tok_i = 0;
	uint8_t item_i = 0;

	char ch;
	while ((ch = fgetc(file)) != EOF) {
		switch (status) {
			case C_NONE: {
				if (ch == 's') {
					setting = S_SRC;
					status = C_SIZE1;
				}
				if (ch == 'o') {
					setting = S_OUT;
					status = C_STUFF;
				}
				break;
			}
			case C_SIZE1: {
				temp_size = (uint8_t)ch;
				temp_size = temp_size << 8;
				status = C_SIZE2;
				break;
			}
			case C_SIZE2: {
				temp_size = temp_size | (uint8_t)ch;
				if (setting == S_SRC) {
					config->src_len = temp_size;
					config->src_array = malloc(config->src_len * MAX_PATH_LEN);
				}
				status = C_STUFF;
				tok_i = 0;
				break;
			}
			case C_STUFF: {
				if (setting == S_SRC) {
					if (ch == '\n' || ch == '\1') {
						config->src_array[item_i * MAX_PATH_LEN + tok_i] = '\0';
						item_i++;
						tok_i = 0;

						if (ch == '\1') {
							status = C_NONE;
						}
						break;
					}
					config->src_array[item_i * MAX_PATH_LEN + tok_i] = ch;
					tok_i++;
					if (tok_i > MAX_PATH_LEN) {
						fprintf(stderr, "one file is nested too deep");
						return 0;
					}
				}
				if (setting == S_OUT) {
					if (ch == '\1') {
						config->out[tok_i] = '\0';
						status = C_NONE;
						tok_i = 0;
						break;
					}
					config->out[tok_i] = ch;
					tok_i++;
				}
				break;
			}
		}
	}

	return 1;
}
