#include <stdlib.h>

unsigned long long fact(long num) {
	if (num > 0) {
		return num * fact(num - 1);
	} else {
		return 1;
	}
}

int main(int argc, char** argv) {
	if (argc != 2) {
		return - 1;
	}

	char *p;
	long num = strtol(argv[1], &p, 10);

	if (*p != '\0') {
		return -1;
	} else {
		return fact(num);
	}
}
