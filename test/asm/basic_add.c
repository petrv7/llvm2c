#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	if (argc != 3) {
		return -1;
	}

	char *p;
	long l = strtol(argv[1], &p, 10);

	if (*p != '\0') {
		return -1;
	}

	long r = strtol(argv[2], &p, 10);

	int result;

	__asm__ __volatile__(
	        "mov    %%rbx, %%rax;   \n\t"
	        "add    %%rcx, %%rax;   \n\t"
	        : "=a" (result)
        	: "b" (l), "c" (r)
        	:
    	);

	return result;
}
