#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char **argv) {

    /* All variables declared outside a parallel region 
       are shared in the parallel region */
    int x = 7;
    int y = 34;
    int *A;

#pragma omp parallel num_threads(4) private(x)
{
    /* all variables declared inside a parallel region 
       are private */
    int tid, nthreads;

    tid = omp_get_thread_num();
    nthreads = omp_get_num_threads();

    if (tid == 0) {
        x = 0;
        y = 3;
    }

    if (tid == 1) {
        x = 1;
        y = 2;
    }

    printf("tid %d, x %d, y %d\n", tid, x, y);

#pragma omp barrier

}

    printf("outside the loop, x %d y %d\n", x, y);

    return 0;
}
