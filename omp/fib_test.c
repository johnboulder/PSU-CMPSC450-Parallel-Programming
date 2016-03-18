#include <stdio.h>
#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif

int fib(int n) {

    int i, j;

    if (n < 2) 
        return n;
    else {

#pragma omp task shared(i)
        i = fib(n-1);

#pragma omp task shared(j)
        j = fib(n-2);

#pragma omp taskwait
        return i+j;
    }
}

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
    }

    int n = atoi(argv[1]);
    int fibterm = fib(n);
    fprintf(stderr, "%dth term in Fib series is %d\n", n, fibterm);
}
