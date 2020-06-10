#include "libminic.h"

int main() {
	write(1, "sleeping ... ", 13);
	sleep(3);
	write(1, "done\n", 5);
	return 0;
}
