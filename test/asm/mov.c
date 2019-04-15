#include <stdio.h>

int main() {
    unsigned long long result;
    __asm__ __volatile__(
        "mov    $42, %%rbx;   \n\t"
        "mov    %%rbx, %0;    \n\t"
        : "=c" (result)
        :
        : "%rbx"
    );

    return result;
}