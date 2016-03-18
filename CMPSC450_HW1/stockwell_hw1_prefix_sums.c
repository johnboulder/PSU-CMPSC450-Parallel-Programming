/* compile the code with gcc -fopenmp stockwell_hw1_prefix_sums.c -lm -o hw*/
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>

void printArray(int32_t *A, int length);

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
	if(argc<3)
	{
		fprintf(stderr, "Needs <n> <threads>\n");
		exit(1);
	}

	// Turn off dynamic teaming
	omp_set_dynamic(0);

	int n, threads;
	n = atoi(argv[1]);
	threads = atoi(argv[2]);

	int *A = malloc(sizeof(int32_t) * n);

	omp_set_num_threads(threads);

	int i;
	int j;
	for(i = 0; i<n; i++)
		A[i] = i+1;

	int k, repeats = 100;
	double elt_avg = 0, elt_sum;
	for(k = 0; k<repeats; k++)
	{
		double elt = timer();
		// Upsweep
		for(i = 0; i<=(int)(log(n-1)/log(2.0)); i++)
		{
			int exp = (int) pow(2.0, (double)i+1);
			int expd = (int) pow(2.0, (double)i);
			#pragma omp parallel for
			for(j = 0; j<n; j=j+exp)
			{
				A[j+exp-1] = A[j+expd-1] + A[j+exp-1];
			}
		}
		// Set the last index to 0
		//  printArray(A, n);
		A[n-1] = 0;

		// Downsweep
		for(i = (int)(log(n-1)/log(2.0));i>=0; i--)
		{
			int exp = (int) pow(2.0, (double)i+1);
			int expd = (int) pow(2.0, (double)i);
			#pragma omp parallel for 
			for(j = 0; j<n; j=j+exp)
			{
				int32_t t = A[j+expd-1]; // first saved
				A[j+expd-1] = A[j+exp-1]; // second stored in first
				A[j+exp-1] = A[j+exp-1]+t; // stored in second
			}
		}

		elt = timer() - elt;
		elt_sum+=elt;
	}
	elt_avg = elt_sum/repeats;
	//printArray(A, n);

	free(A);
	//fprintf(stderr, "n: %d, approx_pi: %.10lf, total sum: %lf\n",n, approx_pi, sum);
	fprintf(stderr, "Elapsed time: %9.6lf s\n", elt_avg);
	// (num_threads*flops_per_thread)/time_taken_in_seconds
	long double flops = ((n/threads)/1e9/elt_avg)*6;
	fprintf(stderr, "GFlops rate: %11.6Lf GFlops\n", flops);

	return 0;
}

void printArray(int32_t *A, int length)
{
	int i;
	for(i = 0; i<length; i++)
	{
		printf("index:%d value:%d\n",i, A[i]);
	}
	printf("\n");
}
