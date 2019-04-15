#include <stdlib.h>

int digitSum(long num) {
	long l = num;
	int rem = 0;
	int sum = 0;

	while (l != 0) {
		rem = l % 10;
		sum += rem;
		l = l / 10;		
	}

	return sum;
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

	return digitSum(num);
}
