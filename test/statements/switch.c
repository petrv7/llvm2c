#include <stdlib.h>

int main(int argc, char** argv) {
	if (argc != 2) {
		return -1;
	}

	char *p;
	long num = strtol(argv[1], &p, 10);

	if (*p != '\0') {
		return -1;
	}

	switch (num) {
	case 0:
		return 1;
	case -5:
		return 10;
	case 2:
		return -1;
	default:
		return 123;
	}
}
