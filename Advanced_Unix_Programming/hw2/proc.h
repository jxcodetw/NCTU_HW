#ifndef _PROC_H_
#define _PROC_H_

#include <unistd.h>
#include <sys/types.h>

struct procinfo_t {
	int pid;
	char state;
	uid_t uid;
	gid_t gid;
	int ppid;
	int pgid;
	int sid;
	int tty_nr;
	int tpgid;
	unsigned int flags;

	unsigned int tty_major;
	unsigned int tty_minor;

	char *image;
	char *cmdline;
	char *tty_dev;
};

struct procinfos_t {
	struct procinfo_t *info;
	int *idx;
	int len;
};

struct procinfos_t proc_parse();
void tty_parse(struct procinfos_t procinfos, int len);

#endif
