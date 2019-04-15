#include <stdlib.h>

long square(long l) {
	return l * l;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		return -1;
	}

	char *p;
	long num = strtol(argv[1], &p, 10);

	if (*p != '\0') {
		return -1;
	}

	long (*ptr)(long) = &square;
	return (*ptr)(num);
}
