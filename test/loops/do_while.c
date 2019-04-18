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

	int i = 0;

	do {
		num += i;
		i++;
	} while (i < 10);

	return num;
}
