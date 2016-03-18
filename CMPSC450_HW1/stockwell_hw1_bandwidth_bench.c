/* compile the code with gcc -O3 stockwell_hw1_bandwidth_bench.c -o hw*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>

static double timer() 
{
    
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) (tp.tv_sec) + 1e-6 * tp.tv_usec);

    /* The code below is for another high resolution timer */
    /* I'm using gettimeofday because it's more portable */
    /*
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return ((double) (tp.tv_sec) + 1e-9 * tp.tv_nsec);
    */
}


int main(int argc, char **argv) 
{

    /* One input argument, value of n */
    if (argc != 2) {
        fprintf(stderr, "%s <n>\n", argv[0]);
        exit(1);
    }

    int n;

    n = atoi(argv[1]);

    assert(n > 0);
    assert(n <= 1000000000);
    /* making n a multiple of 4 */
    n = (n/4) * 4;

    int *A;
    A = (int *) malloc(n * sizeof(int));
    assert(A != 0);

    int i;

    /* initialize values to be 0, 1, 2, 3 */
    for (i=0; i<n; i++) {
        A[i] = (i & 3);
    }

    /* Number of times to run */
    int num_iterations = 10;
    if (n < 100000) {
        num_iterations = 10000;
    } else if (n < 1000000) {
        num_iterations = 1000;
    } else if (n < 10000000) {
        num_iterations = 100;
    }

    long total_sum = 0;
    double elt = timer();
    int iter;
    for ( iter=0; iter<num_iterations; iter++) {
        int sum = 0;
        for (i=0; i<n; i++) {
            sum += A[i];
        }
        total_sum += sum;
    }
    elt = timer() - elt;

    fprintf(stderr, "n: %d, num_iterations: %d, total sum: %ld\n",
            n, num_iterations, total_sum);
    fprintf(stderr, "Elapsed time: %9.6lf s\n", elt);
    fprintf(stderr, "Sustained bandwidth: %9.6lf GB/s\n", 
                    4.0*n*num_iterations/(elt*1e9));
    fprintf(stderr, "sizeof int: %d\n", sizeof(int));
    free(A);

    return 0;
}
