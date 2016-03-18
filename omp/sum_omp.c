#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#ifdef _OPENMP
#include <omp.h>
#endif

static double timer() {
    /* 
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) (tp.tv_sec) + 1e-6 * tp.tv_usec);
    */

    /* The code below is for another high resolution timer */
    /* I'm using gettimeofday because it's more portable */
    
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return ((double) (tp.tv_sec) + 1e-9 * tp.tv_nsec);
    
}


static int sum_serial(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Variant: serial\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

        int i;

        for (i=0; i<n; i++) {
            sum += A[i];
        }

        elt = timer() - elt;
        avg_elt += elt;

        /* correctness check */
        assert(sum == (3*(n/2)));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;

}

static int sum_parfor_reduce(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Variant: parallel for and reduction clause\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

        int i;
#pragma omp parallel for private(i) reduction(+:sum)
        for (i=0; i<n; i++) {
            sum += A[i];
        }

        elt = timer() - elt;
        avg_elt += elt;

        /* correctness check */
        assert(sum == (3*(n/2)));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;


}
    
static int sum_parregion_for_reduce(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Variant: parallel region, omp for, and reduction clause\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

#pragma omp parallel
{

        int i;
        /* We should not use parallel for now */
        /* Just use omp for */
        /* i is local/private because it is declared inside the parallel region */
#pragma omp for reduction(+:sum)
        for (i=0; i<n; i++) {
            sum += A[i];
        }
}

        elt = timer() - elt;
        avg_elt += elt;

        /* correctness check */
        assert(sum == (3*(n/2)));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;


}

static int sum_parregion_for_myreduce(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Variant: parallel region, omp for, and own reduction\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

#pragma omp parallel
{

        int i;
        int part_sum = 0;

#pragma omp for
        for (i=0; i<n; i++) {
            part_sum += A[i];
        }

        /* critical region: only one thread can execute it at a time */

#pragma omp critical
        sum += part_sum;

}

        elt = timer() - elt;
        avg_elt += elt;

        /* correctness check */
        assert(sum == (3*(n/2)));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;



}

    
static int sum_parregion_myparfor_myreduce(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Variant: parallel region, own loop part, and own reduction\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

#pragma omp parallel
{

        int i;
        int part_sum = 0;

        /* This is what you do if you really dont trust the compiler --
           manually partition the loop */

        int tid, nthreads;

#ifdef _OPENMP
        tid = omp_get_thread_num();
        nthreads = omp_get_num_threads();
#else
        tid = 0;
        nthreads = 1;
#endif

        int my_start_i = (n/nthreads)*tid;
        int my_end_i   = (n/nthreads)*(tid+1);

        if (tid == (nthreads-1))
            my_end_i = n;

        for (i=my_start_i; i<my_end_i; i++) {
            part_sum += A[i];
        }

        /* We need a barrier here (after a manually-scheduled loop). 
           A barrier is implicit when we use the omp for pragma. 
           We have to wait until all threads are done before leaving */

#pragma omp barrier

        /* critical region: only one thread can execute it at a time */

#pragma omp critical
        sum += part_sum;

}

        elt = timer() - elt;
        avg_elt += elt;

        /* correctness check */
        assert(sum == (3*(n/2)));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;

}


static int sum_parfor_dynamicsched_reduce(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Variant: parallel for (dynamic schedule) and reduction clause\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

        int i;
#pragma omp parallel for private(i) schedule(dynamic) reduction(+:sum)
        for (i=0; i<n; i++) {
            sum += A[i];
        }

        elt = timer() - elt;
        avg_elt += elt;

        /* correctness check */
        assert(sum == (3*(n/2)));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;

}


static int sum_parregion_for_falseshare_myreduce(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Variant: parallel region, for, and serial reduction with false sharing\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    int nthreads;
    int* part_sums;

    /* Just a small parallel region to get number of threads */
#pragma omp parallel
{

#ifdef _OPENMP
    nthreads = omp_get_num_threads();
#else
    nthreads = 1;
#endif
}

    part_sums = (int *) malloc(nthreads * sizeof(int));
    assert(part_sums != NULL);

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

#pragma omp parallel
{
        int i;
        int tid;

#ifdef _OPENMP
        tid = omp_get_thread_num();
#else
        tid = 0;
#endif

        part_sums[tid] = 0;

#pragma omp for private(i)
        for (i=0; i<n; i++) {
            part_sums[tid] += A[i];
        }

        /* let one thread do the summation now */
        if (tid == 0) {
            for (i=0; i<nthreads; i++) {
                sum += part_sums[i];
            }
        }
}

        elt = timer() - elt;
        avg_elt += elt;

        /* correctness check */
        assert(sum == (3*(n/2)));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    free(part_sums);

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;

}

    
static int sum_parregion_for_nofalseshare_myreduce(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Variant: parallel region, for, and serial reduction (without false sharing)\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    int nthreads;
    int* part_sums;

    /* Just a small parallel region to get number of threads */
#pragma omp parallel
{

#ifdef _OPENMP
    nthreads = omp_get_num_threads();
#else
    nthreads = 1;
#endif
}

    part_sums = (int *) malloc(nthreads * 16 * sizeof(int));
    assert(part_sums != NULL);

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

#pragma omp parallel
{
        int i;
        int tid;

#ifdef _OPENMP
        tid = omp_get_thread_num();
#else
        tid = 0;
#endif

        part_sums[16*tid] = 0;

#pragma omp for private(i)
        for (i=0; i<n; i++) {
            part_sums[16*tid] += A[i];
        }

        /* let one thread do the summation now */
        if (tid == 0) {
            for (i=0; i<nthreads; i++) {
                sum += part_sums[16*i];
            }
        }
}

        elt = timer() - elt;
        avg_elt += elt;

        /* correctness check */
        assert(sum == (3*(n/2)));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    free(part_sums);

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;



}
    
static int sum_parfor_sync_in_loop(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Variant: parallel for and sync inside loop\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

        int i;
#pragma omp parallel for private(i)
        for (i=0; i<n; i++) {
            /* can use an atomic intrinsic here instead, but even that's going to be bad */
#pragma omp critical
            sum += A[i];
        }

        elt = timer() - elt;
        avg_elt += elt;

        assert(sum == (3*(n/2)));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;



}

static int sum_parfor_sharedloopitervar(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Parallel for, iteration variable is shared (compiler converts to local)\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

        int i;
        /* This is okay, but I would recommend marking i as private */
#pragma omp parallel for reduction(+:sum)
        for (i=0; i<n; i++) {
            sum += A[i];
        }

        elt = timer() - elt;
        avg_elt += elt;

        assert(sum == (3*(n/2)));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;


}



static int incorrect_sum_parfor_nosync(const int *A, const int n, const int num_iterations) {

    fprintf(stderr, "N %d\n", n);
    fprintf(stderr, "Buggy code: parallel for, no attempt to reduce\n");
    fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

    int iter;
    double avg_elt;

    avg_elt = 0.0;

    for (iter = 0; iter < num_iterations; iter++) {

        double elt;

        elt = timer();

        int sum = 0;

        int i;
#pragma omp parallel for private(i)
        for (i=0; i<n; i++) {
            sum += A[i];
        }

        elt = timer() - elt;
        avg_elt += elt;

        /* The assert is going to fail, so I'm commenting it out */
        // assert(sum == (3*(n/2)));
        fprintf(stderr, "Computed value %d, actual %d\n", sum, 3*(n/2));
        fprintf(stderr, "%9.3lf\n", elt*1e3);

    }

    avg_elt = avg_elt/num_iterations;

    fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
    fprintf(stderr, "Average read bandwidth: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
    return 0;
}
    

int main(int argc, char **argv) {

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

    /* Why are we using 'parallel for' here? */

    /* initialize values to be 0, 1, 2, 3 */
#pragma omp parallel for private(i)
    for (i=0; i<n; i++) {
        A[i] = (i & 3);
    }

    /* Number of times to run each code variant */
    int num_iterations = 10;

    sum_serial(A, n, num_iterations);

    sum_parfor_reduce(A, n, num_iterations);
    
    sum_parregion_for_reduce(A, n, num_iterations);
    
    sum_parregion_for_myreduce(A, n, num_iterations);

    sum_parregion_myparfor_myreduce(A, n, num_iterations);

    /* very slow, we'll just run once */
    sum_parfor_dynamicsched_reduce(A, n, 1);

    sum_parregion_for_falseshare_myreduce(A, n, num_iterations);
    
    sum_parregion_for_nofalseshare_myreduce(A, n, num_iterations);
    
    /* super slow, we'll just run once */
    sum_parfor_sync_in_loop(A, n, 1);
    
    sum_parfor_sharedloopitervar(A, n, num_iterations);
    
    incorrect_sum_parfor_nosync(A, n, num_iterations);

    free(A);

    return 0;
}
