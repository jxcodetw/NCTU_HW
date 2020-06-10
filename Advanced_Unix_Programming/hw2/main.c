#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>

#include "opt.h"
#include "proc.h"

struct procinfos_t procinfos;

void swap_int(int *a, int *b) {
	int t = *a;
	*a = *b;
	*b = t;
}

int cmp(const void *p1, const void *p2) {
	int i1 = *((int*)p1);
	int i2 = *((int*)p2);
	switch(opts.sorted_by) {
		default:
		case S_PID:
			return procinfos.info[i1].pid - procinfos.info[i2].pid;
		case S_PPID:
			return procinfos.info[i1].ppid - procinfos.info[i2].ppid;
		case S_PGID:
			return procinfos.info[i1].pgid - procinfos.info[i2].pgid;
		case S_SID:
			return procinfos.info[i1].sid - procinfos.info[i2].sid;
	}
	return procinfos.info[i1].pid - procinfos.info[i2].pid;
}

int main(int argc, char *argv[]) {
	errno = 0;
	uid_t uid = getuid();
	opt_parse(argc, argv);
	procinfos = proc_parse();

	/* filter */
	int filtered_len = 0;
	for(int i=0;i<procinfos.len;++i) {
		struct procinfo_t *info = &(procinfos.info[i]);
		bool same_uid = info->uid == uid;
		bool has_tty = info->tty_major != 0 && info->tty_minor != 0;
		if (opts.list_all_users_process) {
			same_uid = true;
		}
		if (opts.list_without_associated_terminal) {
			has_tty = true;
		}
		if (same_uid && has_tty) {
			swap_int(&(procinfos.idx[filtered_len++]), &(procinfos.idx[i]));
		}
	}

	/* sorting */
	qsort(procinfos.idx, filtered_len, sizeof(int), cmp);

	/* get tty name */
	tty_parse(procinfos, filtered_len);

	/* display */
	puts("  pid   uid   gid  ppid  pgid   sid      tty St (img) cmd");
	for(int i=0;i<filtered_len;++i) {
		struct procinfo_t *info = &(procinfos.info[procinfos.idx[i]]);
		printf("%5d %5d %5d %5d %5d %5d %8s %2c %s %s\n", 
			info->pid, info->uid, info->gid, info->ppid, info->pgid, info->sid,
			(info->tty_dev == NULL ? "-" : info->tty_dev),
			info->state, info->image, info->cmdline);
	}

	/* clean up */
	for(int i=0;i<procinfos.len;++i) {
		struct procinfo_t *info = &(procinfos.info[i]);
		free(info->image);
		free(info->cmdline);
		free(info->tty_dev);
	}
	free(procinfos.info);
	free(procinfos.idx);
	return 0;
}
