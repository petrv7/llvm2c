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

	if (num > 10) {
		if (num > 5) {
			return 2;
		} else {
			return 1;
		}	
	} else {
		if (num < -5) {
			return -2;
		} else {
			return -1;
		}	
	}
}
