#include <stdlib.h>

int main(int argc, char** argv) {
	if (argc != ) {
		return -1;
	}

	char *p;
	long num = strtol(argv[1], &p, 10);

	if (*p != '\0') {
		return -1;
	}
}
