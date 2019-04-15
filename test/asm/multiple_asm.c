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

	if (*p != '\0') {
		return -1;
	}

	int result;
	__asm__ ( "addl %%ebx, %%eax;" 
		: "=a" (result) 
		: "a" (l) , "b" (r) 
	);

    	__asm__ ( "subl %%ebx, %%eax;" 
		: "=a" (result) 
		: "a" (result) , "b" (10) 
	);

    	__asm__ ( "imull %%ebx, %%eax;" 
		: "=a" (result) 
		: "a" (result) , "b" (r) 
	);

	return result;
}
