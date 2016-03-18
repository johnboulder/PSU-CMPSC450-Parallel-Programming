#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>

int main(int argc, char **argv) {

    /* create two arrays of size 1 million each */
    int *A;
    A = (int *) malloc(1000000 * sizeof(int));
    assert(A != NULL);
    int *B;
    B = (int *) malloc(1000000 * sizeof(int));
    assert(B != NULL);

    int j;
    for (j=0; j<1000000; j++) {
        A[j] = (j % 8);
    }
    
#pragma omp parallel num_threads(4)
{
    int i;

#pragma omp sections 
{    

#pragma omp section
{
    int sumA = 0;
    for (i=0; i<1000000; i++) {
        sumA += A[i];    
    }
    printf("tid %d, nthreads %d, sum %d\n", 
            omp_get_thread_num(), omp_get_num_threads(), sumA);
}

#pragma omp section
    printf("tid %d, nthreads %d\n", 
            omp_get_thread_num(), omp_get_num_threads()); 
}
}
   
    free(A);
    free(B);

    return 0;
}
