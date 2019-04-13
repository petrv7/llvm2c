#include <stdlib.h>

unsigned long long fact(long num) {
	unsigned long long result = 1;

	for (long l = 1; l <= num; l++) {
		result *= l;	
	}

	return result;
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
