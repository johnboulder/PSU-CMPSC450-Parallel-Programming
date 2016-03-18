/* compile the code with -O3 */
#include <omp.h>
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
	if (argc != 4) {
		fprintf(stderr, "%s <n> <x> <t>\n", argv[0]);
		exit(1);
	}

	int x, n, t;
	n = atoi(argv[1]);
	x = atoi(argv[2]);
	t = atoi(argv[3]);

	assert(x >0);
	assert(n > 0);
	assert(n%2 == 0);

	omp_set_dynamic(0);
	omp_set_num_threads(t);

	int *A = malloc(sizeof(int)*n);

	int i;

	for(i = 0; i<n; i++)
	{
		A[i] = x;
	}

	int total = 1;
	double elt = timer();
	#pragma omp parallel for private(i) reduction(*:total)
	for(i=0; i<n; i++)
	{
		total = total * A[i];
	}

	//---------------------------------------------- OLD code start

	elt = timer() - elt;

	fprintf(stderr, "exponent:%d\n", total);
	fprintf(stderr, "Elapsed time: %9.6lf s\n", elt);
	// (num_threads*flops_per_thread)/time_taken_in_seconds
	long double flops = ((n/t)/1e9/elt)*6;

	fprintf(stderr, "GFlops rate: %11.6Lf GFlops\n", flops);

	return 0;
}
