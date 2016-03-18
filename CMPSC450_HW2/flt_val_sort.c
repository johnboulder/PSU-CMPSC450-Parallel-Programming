#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "qsort.h"

static double timer() {

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

void printArray(int32_t *A, int n)
{
	int i;
	for(i = 0; i<n; i++)
	{
		fprintf(stderr, "%d,", A[i]);
	}
	fprintf(stderr, "\n");
}

/* comparison routine for C's qsort */
static int qs_cmpf(const void *u, const void *v) {

	if (*(int32_t *)u > *(int32_t *)v)
		return 1;
	else if (*(int32_t *)u < *(int32_t *)v)
		return -1;
	else
		return 0;
}

/* inline QSORT() comparison routine */
#define inline_qs_cmpf(a,b) ((*a)<(*b))


static int inline_qsort_serial(const int32_t *A, const int n, const int num_iterations) {

	fprintf(stderr, "N %d\n", n);
	fprintf(stderr, "Using inline qsort implementation\n");
	fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

	int iter;
	double avg_elt;

	int32_t *B;
	B = (int32_t *) malloc(n * sizeof(int32_t));
	assert(B != NULL);

	avg_elt = 0.0;

	for (iter = 0; iter < num_iterations; iter++) {

		int i;

		for (i=0; i<n; i++) {
			B[i] = A[i];
		}

		double elt;
		elt = timer();

		QSORT(int32_t, B, n, inline_qs_cmpf);

		elt = timer() - elt;
		avg_elt += elt;
		fprintf(stderr, "%9.3lf\n", elt*1e3);

		/* correctness check */
		for (i=1; i<n; i++) {
			assert(B[i] >= B[i-1]);
		}

	}

	avg_elt = avg_elt/num_iterations;

	free(B);

	fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
	fprintf(stderr, "Average sort rate: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
	return 0;

}

static int qsort_serial(const int32_t *A, const int n, const int num_iterations) {

	fprintf(stderr, "N %d\n", n);
	fprintf(stderr, "Using C qsort\n");
	fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

	int iter;
	double avg_elt;

	int32_t *B;
	B = (int32_t *) malloc(n * sizeof(int32_t));
	assert(B != NULL);

	avg_elt = 0.0;

	for (iter = 0; iter < num_iterations; iter++) {

		int i;

		for (i=0; i<n; i++) {
			B[i] = A[i];
		}

		double elt;
		elt = timer();

		qsort(B, n, sizeof(int32_t), qs_cmpf);

		elt = timer() - elt;
		avg_elt += elt;
		fprintf(stderr, "%9.3lf\n", elt*1e3);

		/* correctness check */
		for (i=1; i<n; i++) {
			assert(B[i] >= B[i-1]);
		}

	}

	avg_elt = avg_elt/num_iterations;

	free(B);

	fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
	fprintf(stderr, "Average sort rate: %6.3lf MB/s\n", 4.0*n/(avg_elt*1e6));
	return 0;

}

void copyArray(int32_t **A, int32_t **B, int begin, int end)
{
	int i;
	for(i = 0; i<end; i++)
	{
		B[i] = A[i];
	}
}

void merge(int32_t **A, int32_t **B, int begin, int mid, int end)
{
	int i, j = 0, k = 0;
	for(i = begin; i<end; i++)
	{
		if(j<mid && ((*A)[j]<(*A)[k]|| k>=end))
		{
			(*B)[i] = (*A)[j];
			j++;
		}
		else
		{
			(*B)[i] = (*A)[k];
			k++;
		}
	}
}

void mergesort(int32_t *A, int32_t *B, int begin, int end)
{
	if(begin-end < 2)
		return;

	int mid = (begin+end)/2;

	mergesort(A, B, begin, mid);
	mergesort(A, B, mid, end);

	merge(A, B, begin, mid, end);
	copyArray(B, A, begin, end);
}

void mergesortRunner(int32_t *A, int32_t *B, int begin, int end, int num_iterations)
{
	fprintf(stderr, "N %d\n", end);
	fprintf(stderr, "parallel mergesort\n");
	fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

	double avg_elt;
	avg_elt = 0.0;

	int32_t *C;
	C = (int32_t *) malloc(end * sizeof(int32_t));
	assert(B != NULL);

	int i;
	for(i = 0; i<num_iterations; i++)
	{
		int j;
		for(j = 0; j<end; j++)
			C[j] = A[j];
		
		double elt;
		elt = timer();

		mergesort(C, B, begin, end);

		printArray(B, end);

		elt = timer() - elt;
		avg_elt += elt;
		fprintf(stderr, "%9.3lf\n", elt*1e3);

		/* correctness check */
		for (i=1; i<end; i++) {
			assert(B[i] >= B[i-1]);
		}
	}

	avg_elt = avg_elt/num_iterations;

	free(C);

	fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
	fprintf(stderr, "Average sort rate: %6.3lf MB/s\n", 4.0*end/(avg_elt*1e6));
}

/* generate different inputs for testing sort */
int gen_input(int32_t *A, int n, int input_type) {

	int i;

	/* uniform random values */
	if (input_type == 0) {

		srand(123);
		for (i=0; i<n; i++) {
			A[i] = ((int32_t) rand())%81;
		}

		/* sorted values */    
	} else if (input_type == 1) {

		for (i=0; i<n; i++) {
			A[i] = (int32_t) i;
		}

		/* almost sorted */    
	} else if (input_type == 2) {

		for (i=0; i<n; i++) {
			A[i] = (int32_t) i;
		}

		/* do a few shuffles */
		int num_shuffles = (n/100) + 1;
		srand(1234);
		for (i=0; i<num_shuffles; i++) {
			int j = (rand() % n);
			int k = (rand() % n);

			/* swap A[j] and A[k] */
			int32_t tmpval = A[j];
			A[j] = A[k];
			A[k] = tmpval;
		}

		/* array with single unique value */    
	} else if (input_type == 3) {

		for (i=0; i<n; i++) {
			A[i] = 1;
		}

		/* sorted in reverse */    
	} else {

		for (i=0; i<n; i++) {
			A[i] = (int32_t) (n + 1.0 - i);
		}

	}

	return 0;

}


int main(int argc, char **argv) {

	if (argc != 4) {
		fprintf(stderr, "%s <n> <input_type> <alg_type>\n", argv[0]);
		fprintf(stderr, "input_type 0: uniform random\n");
		fprintf(stderr, "           1: already sorted\n");
		fprintf(stderr, "           2: almost sorted\n");
		fprintf(stderr, "           3: single unique value\n");
		fprintf(stderr, "           4: sorted in reverse\n");
		fprintf(stderr, "alg_type 0: use C qsort\n");
		fprintf(stderr, "         1: use inline qsort\n");
		fprintf(stderr, "         2: use mergesort\n");
		exit(1);
	}

	int n;

	n = atoi(argv[1]);

	assert(n > 0);
	assert(n <= 1000000000);

	int32_t *A;
	A = (int32_t *) malloc(n * sizeof(int32_t));
	assert(A != 0);

	int32_t *B = (int32_t *) malloc(n * sizeof(int32_t));

	int input_type = atoi(argv[2]);
	assert(input_type >= 0);
	assert(input_type <= 4);

	gen_input(A, n, input_type);
	copyArray(&A, &B, 0, n);

	int alg_type = atoi(argv[3]);

	int num_iterations = 10;

	assert((alg_type == 0) || (alg_type == 1)|| (alg_type == 2));

	if (alg_type == 0) 
	{
		qsort_serial(A, n, num_iterations);
	} 
	else if (alg_type == 1) 
	{    
		inline_qsort_serial(A, n, num_iterations);
	}
	else if (alg_type == 2) 
	{
		// n must be a power of 2 for parallel to work
		assert(n%2 == 0);
		printArray(B, n);
		mergesortRunner(A, B, 0, n, num_iterations);
	}

	free(A);

	return 0;
}
