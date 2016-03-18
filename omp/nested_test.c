#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char **argv) {

    int x = 0;
    if (argc == 2)
        x = atoi(argv[1]);


#pragma omp parallel num_threads(4)
{
    int tid1 = omp_get_thread_num();
    int nthreads1 = omp_get_num_threads();

#pragma omp parallel num_threads(2)
{
    int tid2 = omp_get_thread_num();
    int nthreads2 = omp_get_num_threads();

    printf("Hello world from thread tid1 %d tid2 %d of %d\n", 
            tid1, tid2, 
            nthreads1*nthreads2);

}
}

    return 0;
}
