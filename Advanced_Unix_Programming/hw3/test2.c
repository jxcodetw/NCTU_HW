#include "libminic.h"

typedef void (*sighandler_t)(int);

/* fill zeros: slow but it works */
static void myzero(void *m, int size) { char *ptr = (char *) m; while(size-- > 0) *ptr++ = '\0'; }

static sighandler_t signal(int s, sighandler_t h) {
	struct sigaction act, old;
	myzero(&act, sizeof(act));
	act.sa_handler = h;
	sigaction(s, &act, &old);
	return old.sa_handler;
}

static void sigint(int s) {
	write(1, "SIGINT received.\n", 17);
}

int main() {
	signal(SIGINT, sigint);
	while(1) pause();
	return 0;
}
