#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "qsort.h"

float *globA;
float *globB;
float *globC;
float *top;
float *bot;

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

void printArray(float *A, int n)
{
	int i;
	fprintf(stderr, "[");
	for(i = 0; i<n; i++)
	{
		fprintf(stderr, "%f", A[i]);
		if(i<n-1)
			fprintf(stderr, ",");

	}
	fprintf(stderr, "]\n");
}

/* comparison routine for C's qsort */
static int qs_cmpf(const void *u, const void *v) {

	if (*(float *)u > *(float *)v)
		return 1;
	else if (*(float *)u < *(float *)v)
		return -1;
	else
		return 0;
}

/* inline QSORT() comparison routine */
#define inline_qs_cmpf(a,b) ((*a)<(*b))


static int inline_qsort_serial(const float *A, const int n, const int num_iterations) {

	fprintf(stderr, "N %d\n", n);
	fprintf(stderr, "Using inline qsort implementation\n");
	fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

	int iter;
	double avg_elt;

	float *B;
	B = (float *) malloc(n * sizeof(float));
	assert(B != NULL);

	avg_elt = 0.0;

	for (iter = 0; iter < num_iterations; iter++) {

		int i;

		for (i=0; i<n; i++) {
			B[i] = A[i];
		}

		double elt;
		elt = timer();

		QSORT(float, B, n, inline_qs_cmpf);

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

static int qsort_serial(const float *A, const int n, const int num_iterations) {

	fprintf(stderr, "N %d\n", n);
	fprintf(stderr, "Using C qsort\n");
	fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

	int iter;
	double avg_elt;

	float *B;
	B = (float *) malloc(n * sizeof(float));
	assert(B != NULL);

	avg_elt = 0.0;

	for (iter = 0; iter < num_iterations; iter++) {

		int i;

		for (i=0; i<n; i++) {
			B[i] = A[i];
		}

		double elt;
		elt = timer();

		qsort(B, n, sizeof(float), qs_cmpf);

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

void copyArray(float *A, float *B, int begin, int end)
{
	int i;
	
#ifdef _OPENMP
	#pragma omp for
#endif
	for(i = 0; i<end; i++)
	{
		B[i] = A[i];
	}
}

void merge(int begin, int mid, int end)
{
	int i, j = begin, k = mid;
	for(i = begin; i<end; i++)
	{
		if(j<mid && (globC[j]<globC[k]|| k>=end))
		{
			globB[i] = globC[j];
			j++;
		}
		else
		{
			globB[i] = globC[k];
			k++;
		}
	}
}
#ifdef _OPENMP
void p_merge(int begin, int mid, int end)
{
	int i, j = begin, k = mid;

	// take the two halfs and into two threads
	// let them respectively find the highest and lowest values of the two halfs
	
	//top
	#pragma omp task private(i, j, k)
	{
		j = mid-1; 
		k = end-1;

		for(i = end-mid-1; i>=0; i--)
		{
			if(j>begin && (globC[j]>globC[k] || k<=mid))
			{
				top[i] = globC[j];
				j--;
			}
			else
			{
				top[i] = globC[k];
				k--;
			}
		}
	}
	// bot
	#pragma omp task private(i, j, k)
	{
		j = begin;
		k = mid;
		for(i = 0; i<mid-begin; i++)
		{
			if(j<mid && (globC[j]<globC[k] || k>=end))
			{
				bot[i] = globC[j];
				j++;
			}
			else
			{
				bot[i] = globC[k];
				k++;
			}
			
		}
	}
	#pragma omp taskwait
	{
		// Copy the bottom and top to globB
	
		// top
		#pragma omp parallel for private(i)
		for(i = mid; i<end; i++)
		{
			globB[i] = top[i - mid];
		}

		// bot
		#pragma omp parallel for private(i)
		for(i = begin; i<mid; i++)
		{
			globB[i] = bot[i - begin];
		}
	}
}
#endif

void mergesort(int begin, int end)
{
	if(end-begin < 2)
		return;

	int mid = (begin+end)/2;
#ifdef _OPENMP
	#pragma omp task
#endif
	mergesort(begin, mid);
#ifdef _OPENMP
	#pragma omp task
#endif
	mergesort(mid, end);

	//#pragma omp taskwait
#ifdef _OPENMP
	if(end-begin >= 1024)
		p_merge(begin, mid, end);
	else 
		merge(begin, mid, end);
#else
	merge(begin, mid, end);
#endif
	copyArray( globB, globC, begin, end);
}

void mergesortRunner(int begin, int end, int num_iterations)
{
	fprintf(stderr, "N %d\n", end);
	fprintf(stderr, "parallel mergesort\n");
	fprintf(stderr, "Execution times (ms) for %d iterations:\n", num_iterations);

	double avg_elt;
	avg_elt = 0.0;


	globC = (float *) malloc(end * sizeof(float));
	assert(globB != NULL);

	int i = 0;
	for(i=0; i<num_iterations; i++)
	{
		int j;
		for(j = 0; j<end; j++)
			globC[j] = globA[j];

		double elt;
		elt = timer();

		mergesort(begin, end);

		//printArray(globB, end);
		//printArray(globC, end);

		elt = timer() - elt;
		avg_elt += elt;
		fprintf(stderr, "%9.3lf\n", elt*1e3);

		/* correctness check */
		int k; 
		for (k=1; k<end; k++) 
		{
			assert(globB[k] >= globB[k-1]);
		}
	}

	avg_elt = avg_elt/num_iterations;

	free(globC);

	fprintf(stderr, "Average time: %9.3lf ms.\n", avg_elt*1e3);
	fprintf(stderr, "Average sort rate: %6.3lf MB/s\n", 4.0*end/(avg_elt*1e6));
}

/* generate different inputs for testing sort */
int gen_input(float *A, int n, int input_type) {

	int i;

	/* uniform random values */
	if (input_type == 0) {

		srand(123);
		for (i=0; i<n; i++) {
			A[i] = ((float) rand())/n;
		}

		/* sorted values */    
	} else if (input_type == 1) {

		for (i=0; i<n; i++) {
			A[i] = (float) i;
		}
		
		/* almost sorted */    
	} else if (input_type == 2) {

		for (i=0; i<n; i++) {
			A[i] = (float) i;
		}

		/* do a few shuffles */
		int num_shuffles = (n/100) + 1;
		srand(1234);
		for (i=0; i<num_shuffles; i++) {
			int j = (rand() % n);
			int k = (rand() % n);

			/* swap A[j] and A[k] */
			float tmpval = A[j];
			A[j] = A[k];
			A[k] = tmpval;
		}

		/* array with single unique value */    
	} else if (input_type == 3) {

		for (i=0; i<n; i++) {
			A[i] = 1.0;
		}

		/* sorted in reverse */    
	} else {

		for (i=0; i<n; i++) {
			A[i] = (float) (n + 1.0 - i);
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

	float *A;
	A = (float *) malloc(n * sizeof(float));
	assert(A != 0);

	float *B = (float *) malloc(n * sizeof(float));
	
	int input_type = atoi(argv[2]);
	assert(input_type >= 0);
	assert(input_type <= 4);

	gen_input(A, n, input_type);
	gen_input(B, n, 3);

	int alg_type = atoi(argv[3]);

	int num_iterations = 10;

	assert((alg_type == 0) || (alg_type == 1)|| (alg_type == 2));
#ifdef _OPENMP
#pragma omp parallel
{
#pragma omp task
	top = (float *) malloc(n * sizeof(float));
#pragma omp task
	bot = (float *) malloc(n * sizeof(float));
}
#endif
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
		globB = B;
		globA = A;
		//printArray(globB, n);
		mergesortRunner(0, n, num_iterations);
	}

	free(A);
	free(B);
#ifdef _OPENMP
	free(top);
	free(bot);
#endif
	return 0;
}
