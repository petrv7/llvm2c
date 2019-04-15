#include <stdlib.h>

struct one {
	int i;
};

struct two {
	struct one o;
};

int main(int argc, char** argv) {
	if (argc != 2) {
		return -1;
	}

	char *p;
	long num = strtol(argv[1], &p, 10);

	if (*p != '\0') {
		return -1;
	}

	struct two test;
	test.o.i = num;

	return test.o.i;
}
