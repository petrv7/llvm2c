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

	int test = 0;

	for (int i = 0; i < num; i++) {
		test += i;
	}

	return test;
}
