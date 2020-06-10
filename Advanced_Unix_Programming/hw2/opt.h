#ifndef _OPT_H_
#define _OPT_H_

#include <stdbool.h>

enum sorted_by_t {
	S_PID,
	S_PPID,
	S_PGID,
	S_SID,
};

struct opts_t {
	bool list_all_users_process;
	bool list_without_associated_terminal;
	enum sorted_by_t sorted_by;
};

extern struct opts_t opts;

void opt_parse(int argc, char* argv[]);

#endif
