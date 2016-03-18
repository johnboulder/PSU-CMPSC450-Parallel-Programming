#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

void some_other_func(int);

static void static_func(int i) {
    printf("hello from static_func %d\n", i);
}

int main(int argc, char **argv) 
{

    int x = 0;
    if (argc == 2)
        x = atoi(argv[1]);

    int y = 2;

    if (x > 0) 
    {

	#pragma omp parallel if ((x == 4) && (y == 2)) 
	{
		int tid, nthreads;
		tid = omp_get_thread_num();
    		nthreads = omp_get_num_threads();

   		static_func(tid);
    		//some_other_func(tid);

    		printf("Hello world from thread %3d of %3d\n", tid, nthreads);

		//#pragma omp barrier

	}

    }

    return 0;
}
