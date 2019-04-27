#include <string.h>

int main () {
   const char test1[20] = "memcpytest";
   char test2[20];
   memcpy(test2, test1, strlen(test1)+1);
   printf("%s\n", test2);
   
   return(0);
}