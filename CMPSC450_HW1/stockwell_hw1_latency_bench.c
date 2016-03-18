/* compile the code with gcc stockwell_hw1_latency_bench.c -o hw*/
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

	//srand(time(NULL));

	/* making n a multiple of 4 */
	n = (n/4) * 4;

	int *A;
	A = (int *) malloc(n * sizeof(int));
	assert(A != 0);

	int i;

	/* initialize values to be i*/
	for (i=0; i<n; i++) {
		A[i] = i;
	}

	/* Swap some values around*/
	for(i=0; i<2*n; i++)
	{
		int j = (int) random()%(n-1);
		int k = (int) random()%(n-1);
		int temp = A[j];
		A[j] = A[k];
		A[k] = temp;
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

	double elt_sum = 0, elt_avg = 0;
	int k, repeats = 20;
	for(k = 0; k<repeats; k++)
	{
		double elt = timer();
		int iter;
		for ( iter=0; iter<num_iterations; iter++) {
			int sum = 0;
			for (i=0; i<n; i++) {
				int j = A[i];
				int val = (A[j]&3);
				sum = sum+val;
			}
			total_sum += sum;
		}
		elt = timer() - elt;
		elt_sum+=elt;
	}
	elt_avg = elt_sum/repeats;
	// fprintf(stderr, "n: %d, num_iterations: %d, total sum: %ld\n",n, num_iterations, total_sum);
	fprintf(stderr, "Elapsed time: %9.6lf s\n", elt_avg);
	fprintf(stderr, "Sustained latency: %9.6lf mS/MR\n", 
			(elt_avg)/n*num_iterations);
	//fprintf(stderr, "sizeof int: %d\n", sizeof(int));
	free(A);

	return 0;
}
