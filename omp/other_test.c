#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>

int main(int argc, char **argv) {

   
#pragma omp parallel num_threads(4)
{

    int tid = omp_get_thread_num();
    int nthreads = omp_get_num_threads();

#pragma omp single
{    
    fprintf(stderr, "thread %d of %d in single block\n",
            tid, nthreads);
}

#pragma omp master
{    
    fprintf(stderr, "thread %d of %d in master block\n",
            tid, nthreads);
}

}
   
    return 0;
}
