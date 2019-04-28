#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main () {
   char *str;

   str = (char *) malloc(20);
   strcpy(str, "malloctest");
   printf("%s\n", str);
   str = (char *) realloc(str, 40);
   strcat(str, " realloctest");
   printf("%s", str);

   free(str);
   
   return 0;
}
