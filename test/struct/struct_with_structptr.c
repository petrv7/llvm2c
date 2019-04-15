#include <stdlib.h>

struct test {
	int i;
};

struct sptr {
	struct test* ptr;
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

	struct test t = {num};
	struct sptr s = {&t};

	return s.ptr->i;
}
