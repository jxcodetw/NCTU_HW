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
#include <linux/kdev_t.h>

#include "opt.h"
#include "listdir.h"
#include "path.h"
#include "proc.h"

#define MAX_READ 4096

struct array_t {
	void *ptr;
	int len;
};

static char file_content[MAX_READ];

// helper functions

static bool is_num(char *path) {
	while(*path != '\0') {
		if (!(*path >= '0' && *path <= '9')) {
			return false;
		}
		path++;
	}
	return true;
}

static char *skipnchar(char *buf, char c, int n) {
	int cnt = 0;
	for(int i=0;i<strlen(buf);++i) {
		if (cnt >= n) {
			return &(buf[i]);
		}
		if (buf[i] == c) {
			cnt++;
		}
	}
	return buf;
}

static void read_procfs(char *path) {
	int fd = open(path, O_RDONLY), size = 0;
	if (fd == -1) {
		puts(path);
		perror("open() error");
		exit(EXIT_FAILURE);
	}

	size = read(fd, file_content, MAX_READ - 1);
	close(fd);

	if (size == -1) {
		puts(path);
		perror("read() error");
		exit(EXIT_FAILURE);
	}

	// for cmdline
	for(int i=0;i<size;++i) {
		if (file_content[i] == '\0') {
			file_content[i] = ' ';
		}
	}
	file_content[size] = '\0';
}

static void read_status(struct procinfo_t *info, char *path) {
	char *status_path = path_join(path, "status");
	read_procfs(status_path);
	free(status_path);

	char *uidline = skipnchar(file_content, '\n', 8);
	char *gidline = skipnchar(uidline, '\n', 1);
	sscanf(uidline, "%*s%d", &(info->uid));
	sscanf(gidline, "%*s%d", &(info->gid));
}

static void read_stat(struct procinfo_t *info, char *path) {
	char *stat_path = path_join(path, "stat");
	read_procfs(stat_path);
	free(stat_path);

	char *image = malloc(MAX_READ);
	sscanf(file_content, "%d%s %c%d%d%d%d%d%u", 
		&(info->pid), image, &(info->state),
		&(info->ppid), &(info->pgid), &(info->sid),
		&(info->tty_nr), &(info->tpgid), &(info->flags));
	info->image = strdup(image);
	info->tty_major = MAJOR(info->tty_nr);
	info->tty_minor = MINOR(info->tty_nr);
	free(image);
}

static void read_cmdline(struct procinfo_t *info, char *path) {
	char *cmdline_path = path_join(path, "cmdline");
	read_procfs(cmdline_path);
	free(cmdline_path);
	info->cmdline = strdup(file_content);
}

static struct array_t get_proc_queue() {
	struct array_t results = {
		.ptr = NULL,
		.len = 0,
	};
	dirent_node_t *queue = listdir("/proc");
	dirent_node_t *proc_queue = NULL;
	while(queue != NULL) {
		if (is_num(queue->d_name)) {
			dirent_node_t *next = queue->next;
			proc_queue = link_node(proc_queue, queue);
			queue = next;
			results.len += 1;
		} else {
			queue = del_node(queue);
		}
	}
	results.ptr = (void*)proc_queue;

	return results;
}

struct procinfos_t proc_parse() {
	struct procinfos_t procinfos = {
		.info = NULL,
		.idx = NULL,
		.len = 0,
	};
	struct array_t proc_queue = get_proc_queue();
	dirent_node_t *queue = (dirent_node_t*)proc_queue.ptr;
	procinfos.len = proc_queue.len;

	procinfos.info = malloc(sizeof(struct procinfo_t) * procinfos.len);
	procinfos.idx = malloc(sizeof(int) * procinfos.len);
	for(int i=0;i<procinfos.len;++i) {
		struct procinfo_t *info = &(procinfos.info[i]);
		read_stat(info, queue->path);
		read_status(info, queue->path);
		read_cmdline(info, queue->path);
		info->tty_dev = NULL;
		queue = del_node(queue);

		procinfos.idx[i] = i;
	}
	return procinfos;
}

struct devinfo_t {
	unsigned int major;
	unsigned int minor;
	char *name;
	struct devinfo_t *next;
};

static struct array_t parse_dev() {
	dirent_node_t* queue = listdir("/dev");
	struct array_t results;
	struct devinfo_t* devs = NULL;
	results.len = 0;
	while(queue != NULL) {
		//if (S_ISDIR(queue->st.st_mode)) {
		if (queue->is_dir) {
			/* prevent self reference */
			if (strcmp(queue->path, "/dev/fd") == 0 ||
				strcmp(queue->path, "/dev/tty") == 0) {
				queue = del_node(queue);
			} else {
				dirent_node_t* recur_queue = listdir(queue->path);
				queue = del_node(queue);
				while(recur_queue != NULL) {
					dirent_node_t* next = recur_queue->next;
					queue = link_node(queue, recur_queue);
					recur_queue = next;
				}
			}
		} else if (queue->is_chr) {
			struct devinfo_t *dev_node = malloc(sizeof(struct devinfo_t));
			dev_node->major = MAJOR(queue->st.st_rdev);
			dev_node->minor = MINOR(queue->st.st_rdev);
			dev_node->name = strdup(queue->path + 5); /* len("/dev/") = 5 */
			dev_node->next = devs;
			devs = dev_node;
			queue = del_node(queue);
			results.len += 1;
		} else {
			queue = del_node(queue);
		}
	}
	struct devinfo_t *devs_arr = malloc(sizeof(struct devinfo_t) * results.len);
	for(int i=0;i<results.len;++i) {
		devs_arr[i].major = devs->major;
		devs_arr[i].minor = devs->minor;
		devs_arr[i].name = devs->name;

		struct devinfo_t *next = devs->next;
		free(devs);
		devs = next;
	}
	results.ptr = (void*)devs_arr;
	return results;
}

void tty_parse(struct procinfos_t procinfos, const int len) {
	struct array_t devs = parse_dev();
	struct devinfo_t *devs_arr = devs.ptr;

	for(int i=0;i<len;++i) {
		struct procinfo_t *info = &(procinfos.info[procinfos.idx[i]]);
		for(int j=0;j<devs.len;++j) {
			if (info->tty_major == devs_arr[j].major && info->tty_minor == devs_arr[j].minor) {
				info->tty_dev = strdup(devs_arr[j].name);
				break;
			}
		}
	}

	free(devs.ptr);
}
