#include <stdlib.h>

int isPrime(long num) {
	if (num < 2) {
		return 0;
	}
	
	for (int i = 2; i < num / 2; i++) {
		if (num % i == 0) {
			return 0;
		}
	}

	return 1;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		return - 1;
	}

	char *p;
	long num = strtol(argv[1], &p, 10);

	if (*p != '\0') {
		return -1;
	}

	return isPrime(num);
}
