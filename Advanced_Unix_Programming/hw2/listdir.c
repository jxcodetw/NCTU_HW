#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>

#include "path.h"
#include "listdir.h"

typedef dirent_node_t node_t;

static node_t* new_node(char *path);
node_t* del_node(node_t *path);

static node_t* new_node(char *path) {
	node_t *node = malloc(sizeof(node_t));
	if (stat(path, &(node->st)) != 0) {
		puts(path);
		perror("stat() error");
		del_node(node);
		return NULL;
	}
	node->path = strdup(path);
	node->next = NULL;
	return node;
}

node_t* del_node(node_t *node) {
	if (node == NULL) {
		return NULL;
	}
	node_t *next = node->next;
	free(node->path);
	free(node);
	return next;
}

node_t* link_node(node_t *root, node_t *node) {
	node->next = root;
	return node;
}

node_t* listdir(const char const *path) {
	DIR *dp = opendir(path);
	if (dp == NULL) {
		puts(path);
		perror("opendir() error");
		return NULL;
	}

	node_t* results = NULL;
	struct dirent* dirp;
	while((dirp = readdir(dp))) {
		if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) {
			continue;
		}
		char *newpath = path_join(path, dirp->d_name);
		node_t* node = new_node(newpath);
		node->is_dir = dirp->d_type == DT_DIR;
		node->is_chr = dirp->d_type == DT_CHR;
		node->d_name = strdup(dirp->d_name);
		results = link_node(results, node);
	}
	closedir(dp);

	return results;
}
