/* compile the code with gcc -fopenmp stockwell_hw1_pi_omp.c -lm -o hw*/
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
	if (argc != 3) {
		fprintf(stderr, "%s <n> <threads>\n", argv[0]);
		exit(1);
	}

	int t;
	int n;
	n = atoi(argv[1]);
	t = atoi(argv[2]);

	assert(n > 0);
	assert(n <= 1000000000);// 1,000,000,000

	double delta_x = 1.0/n;

	long total_sum = 0;
	double sum = 0.0;
	double x = 0.0;
	int i = 0;
	// Number of threads
	omp_set_num_threads(t);
	int tid; 

	double elt = timer();
	#pragma omp parallel for private(i) reduction(+:sum)
	for(i=0; i<n; i++)
	{
		x = (i+0.5) * delta_x;
		sum = sum + 4.0/(1.0+x*x);
	}

	//---------------------------------------------- OLD code start

	elt = timer() - elt;
	double approx_pi = sum*delta_x;

	fprintf(stderr, "n: %d, approx_pi: %.10lf, total sum: %lf\n",
			n, approx_pi, sum);
	fprintf(stderr, "Elapsed time: %9.6lf s\n", elt);
	// (num_threads*flops_per_thread)/time_taken_in_seconds
	long double flops = ((n/t)/1e9/elt)*6;

	fprintf(stderr, "GFlops rate: %11.6Lf GFlops\n", flops);

	return 0;
}
