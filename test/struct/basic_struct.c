#include <stdlib.h>

struct t {
	int i;
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

	struct t test = {num};
	return test.i;
}
