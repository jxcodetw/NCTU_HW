#include <stdbool.h>
#include <unistd.h>
#include "opt.h"

const static char options[] = "axprqs";

struct opts_t opts = {
	.list_all_users_process = false,
	.list_without_associated_terminal = false,
	.sorted_by = S_PID,
};

void opt_parse(int argc, char* argv[]) {
	int opt;
	while((opt = getopt(argc, argv, "axpqrs")) != -1) {
		switch(opt) {
			case 'a':
				opts.list_all_users_process = true;
				break;
			case 'x':
				opts.list_without_associated_terminal = true;
				break;
			case 'p':
				opts.sorted_by = S_PID;
				break;
			case 'q':
				opts.sorted_by = S_PPID;
				break;
			case 'r':
				opts.sorted_by = S_PGID;
				break;
			case 's':
				opts.sorted_by = S_SID;
				break;
			default:
				/* ignore unknown flags */
				break;
		}
	}
}
