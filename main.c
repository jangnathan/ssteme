#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "helper.h"
#include "parse.h"
#include "cache.h"

#include <sys/stat.h>
#include <sys/types.h>

void create_dir(char *name) {
	mode_t permissions = S_IRWXU;

	uint8_t result = mkdir(name, permissions);

	if (result == 0) {
		return;
	} else {
		if (errno == EEXIST) {
			return;
		}
		fprintf(stderr, "error creating new directory");
	}
}

void path2outname(char *path) {
	uint8_t len = strlen(path);
	for (uint8_t y = 0; y < len; y++) {
		if (path[y] == '/') {
			path[y] = '+';
		}
	}
}

void compile_project(config_t *config, enum COMP_FLAG comp_flag) {
	create_dir(BUILD_DIR_NAME);
	create_dir(OBJECTS_PATH);
	
	char cmd_buf[256];

	for (uint16_t i = 0; i < config->src_len; i++) {
		strcpy(cmd_buf, "gcc -c ");
		if (comp_flag == RELEASE) {
			strcat(cmd_buf, "-O2 ");
		}
		char path[MAX_PATH_LEN];
		strncpy(path, config->src_array + i * MAX_PATH_LEN, MAX_PATH_LEN);
		strcat(cmd_buf, path);
		strcat(cmd_buf, ".c -o ");
		strcat(cmd_buf, OBJECTS_PATH);
		strcat(cmd_buf, "/");
		path2outname(path);
		strcat(cmd_buf, path);
		strcat(cmd_buf, ".o");
		system(cmd_buf);
	}
}

uint8_t cache() {
	parsed_t parsed;
	parse_init(&parsed);

	printf("updating cache...\n");
	create_dir(BUILD_DIR_NAME);
	if (!parse_file(&parsed)) {
		free(parsed.src.array);
		return 0;
	}
	if (!cache_parsed(&parsed)) return 0;
	free(parsed.src.array);
	return 1;
}

void clear() {
	char cmd_buf[8 + strlen(BUILD_DIR_NAME)];
	strcpy(cmd_buf, "rm -rf ");
	strcat(cmd_buf, BUILD_DIR_NAME);
	system(cmd_buf);
}

uint8_t is_root() {
	FILE *file;
	file = fopen("ssteme.cfg", "r");
	if (file != NULL) {
		fclose(file);
		return 1;
	};
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		fprintf(stderr, "needs an argument");
		return 1;
	}

	if (!is_root()) {
		fprintf(stderr, "cwd must be at root; cannot find config file");
		return 1;
	}
	char *command = argv[1];

	char cmd_buf[256];

	config_t config;
	enum COMP_FLAG comp_flag = DEBUG;
	strcpy(config.out, "out");

	if (argv[2] != NULL) {
		if (strcmp(argv[2], "release") == 0) {
			printf("COMPILING RELEASE\n");
			comp_flag = RELEASE;
		}
	}

	if (strcmp(command, "build") == 0) {
		printf("building...\n");
		if (!file_exists(CACHE_PATH)) {
			if (!cache()) return 1;
		}
		cache2config(&config);
		compile_project(&config, comp_flag);
		strcpy(cmd_buf, "gcc -o ");
		strcat(cmd_buf, BUILD_DIR_NAME);
		strcat(cmd_buf, "/");
		strcat(cmd_buf, config.out);
		strcat(cmd_buf, " ");
		strcat(cmd_buf, OBJECTS_PATH);
		strcat(cmd_buf, "/*.o");
		system(cmd_buf);
	} else if (strcmp(command, "compile") == 0) {
		if (!file_exists(CACHE_PATH)) {
			if (!cache()) return 1;
		}
		cache2config(&config);
		compile_project(&config, comp_flag);
	} else if (strcmp(command, "hydrate") == 0) {
		create_dir(BUILD_DIR_NAME);
		create_dir(OBJECTS_PATH);
		if (argv[2] == NULL) return 1;
		char *path = argv[2];
		if (strlen(path) > MAX_PATH_LEN) {
			fprintf(stderr, "path too long");
			return 1;
		}
		if (!file_exists(path)) {
			fprintf(stderr, "file doesnt exist");
			return 1;
		}
		if (!is_valid_src(path)) {
			fprintf(stderr, "invalid source name");
			return 1;
		}
		strcpy(cmd_buf, "gcc -c ");
		strcat(cmd_buf, path);
		strcat(cmd_buf, " -o ");
		strcat(cmd_buf, OBJECTS_PATH);
		strcat(cmd_buf, "/");
		path2outname(path);
		strcat(cmd_buf, path);
		strcat(cmd_buf, ".o");
		system(cmd_buf);
	} else if (strcmp(command, "link") == 0) {
		cache2config(&config);
		strcpy(cmd_buf, "gcc -o ");
		strcat(cmd_buf, BUILD_DIR_NAME);
		strcat(cmd_buf, "/");
		strcat(cmd_buf, config.out);
		strcat(cmd_buf, " ");
		if (comp_flag == RELEASE) {
			strcat(cmd_buf, "-O2 ");
		}
		strcat(cmd_buf, OBJECTS_PATH);
		strcat(cmd_buf, "/*.o");
		system(cmd_buf);
	} else if (strcmp(command, "cache") == 0) {
		if (!cache()) return 1;
	} else if (strcmp(command, "clear") == 0) {
		clear();
	} else if (strcmp(command, "refresh") == 0) {
		clear();
		if (!cache()) return 1;
	} else if (strcmp(command, "print") == 0) {
		printf("printing...\n");
		parsed_t parsed;
		parse_init(&parsed);

		char *what = argv[2];
		if (what == NULL) {
			uint8_t result = parse_file(&parsed);
			if (!result) return 1;
			printf("no. items: %d\n", parsed.src.len);
			for (int i = 0; i < parsed.src.len; i++) {
				printf("%s ", parsed.src.array + i * MAX_PATH_LEN);
			}
			printf("DONE!");
			free(parsed.src.array);
			return 1;
		}
		if (strcmp(what, "cache") == 0) {
			cache2config(&config);

			for (int i = 0; i < config.src_len; i++) {
				printf("%s\n", config.src_array + i * MAX_PATH_LEN);
			}
			free(config.src_array);
		}
		return 0;
	} else {
		fprintf(stderr, "command doesnt exist");
		return 1;
	}
	return 0;
}
