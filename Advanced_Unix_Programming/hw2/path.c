#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "path.h"

static bool starts_with_slash(char const *path) {
	return path[0] == '/';
}

static bool ends_with_slash(char const *path) {
	return path[strlen(path) - 1] == '/';
}

char* path_join(char const *p1, char const *p2) {
	if (starts_with_slash(p2)) {
		p2 += 1;
	}

	int len1 = strlen(p1);
	int len2 = strlen(p2);

	if (ends_with_slash(p1)) {
		len1 -= 1;
	}

	char *path = malloc(len1 + 1 + len2 + 1); /* /dir / file \0 */
	path[0] = '\0';
	strncat(path, p1, len1);
	strncat(path, "/", 1);
	strncat(path, p2, len2);
	return path;
}
