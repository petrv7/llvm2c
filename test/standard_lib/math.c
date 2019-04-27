#include <stdio.h>
#include <math.h>

int main () {
   float f = 123;
   f = log10(f);
   f = pow(f, 2);

   printf("%.1lf\n", floor(f));
   
   return(0);
}