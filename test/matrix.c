#include "syscall.h"

#define N 20

int
main()
{
    int i;
    int result[N];
    result[0] = 0;
    result[1] = 1;
    for (i = 2; i < N; i++)
    {
        result[i] = result[i-1] + result[i-2];
    }
    Halt();
}