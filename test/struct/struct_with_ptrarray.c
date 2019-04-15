#include <stdlib.h>

struct test {
	int i;
};

struct arrptr {
	struct test (*ptr)[1];
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
	struct test arr[1] = {t};
	struct arrptr aptr = {arr};

	return (*aptr.ptr)[0].i;
}
