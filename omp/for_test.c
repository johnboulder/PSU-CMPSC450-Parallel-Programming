#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>

int main(int argc, char **argv) {

    /* create an array of size 1 million */
    int *A;
    A = (int *) malloc(1000000 * sizeof(int));
    assert(A != NULL);
    int j;
    for (j=0; j<1000000; j++) {
        A[j] = (j % 8);
    }
    
    int sum = 0;

#pragma omp parallel num_threads(4)
{

    /* loop iteration variable should always be local */
    int i;
    int tid = omp_get_thread_num();

    //int sum_local = 0;

    /* "omp for" is a work-sharing construct */
#pragma omp for 
    for (i=0; i<1000000; i++) {
#pragma omp critical
        sum += A[i];
    }

    printf("tid %d, sum %d\n", tid, sum);

}

    printf("outside the loop, sum %d\n", 
            sum);
    
    free(A);

    return 0;
}
