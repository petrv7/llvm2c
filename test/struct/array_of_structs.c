#include <stdlib.h>

struct test {
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

	struct test arr[5];

	for (int i = 0; i < 5; i++) {
		arr[i].i = num + i;
	}

	int sum = 0;

	for (int i = 0; i < 5; i++) {
		sum += arr[i].i;
	}

	return sum;
}
