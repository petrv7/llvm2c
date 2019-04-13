#include <stdlib.h>

int main(int argc, char** argv) {
	if (argc != 2) {
		return - 1;
	}

	char *p;
	long num = strtol(argv[1], &p, 10);

	if (*p != '\0') {
		return -1;
	} else {
		int i = num;
		i = i | 123;
		i = i & 111;
		return i;
	}
}
