#ifndef _LISTDIR_H_
#define _LISTDIR_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>

typedef struct dirent_node_t {
	char *path;
	bool is_dir;
	bool is_chr;
	char *d_name;
	struct stat st;
	struct dirent_node_t *next;
} dirent_node_t;

dirent_node_t* listdir(const char const *path);
dirent_node_t* del_node(dirent_node_t* node);
dirent_node_t* link_node(dirent_node_t* root, dirent_node_t* node);

#endif
