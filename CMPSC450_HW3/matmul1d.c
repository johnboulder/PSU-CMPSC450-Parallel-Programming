#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>

#define USE_MPI 1

#if USE_MPI
#include <mpi.h>
#endif

// Given three NxN matrices A, B, and C, your task is to develop optimized serial and parallel versions of C = C + AB.
// Fix N to 5000. Initialize values in matrices A, B, and C to random double-precision floating-point values in [0, 1]. 

void printMatrices(float *A, float *B, int n, int n_p)
{
	fprintf(stderr, "A:\n");
	int i = 0; 
	for(i = 0; i<n*n_p; i++)
	{
		fprintf(stderr, "%.0f ", A[i]);

		if((i+1)%n == 0)
			fprintf(stderr,"\n");
	}

	fprintf(stderr, "B:\n");
	for(i = 0; i<n*n_p; i++)
	{
		fprintf(stderr, "%.0f ", B[i]);

		if((i+1)%n == 0)
			fprintf(stderr,"\n");
	}
}

void printMatrix(float *A, int n, int n_p)
{
	fprintf(stderr, "C:\n");
	int i = 0; 
	for(i = 0; i<n*n_p; i++)
	{
		fprintf(stderr, "%.0f ", A[i]);

		if((i+1)%n == 0)
			fprintf(stderr,"\n");
	}
}

static double timer() 
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) (tp.tv_sec) + 1e-6 * tp.tv_usec);
}

int main(int argc, char **argv) 
{

    int rank, num_tasks;

    /* Initialize MPI */
#if USE_MPI
    MPI_Status Stat;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // printf("Hello world from rank %3d of %3d\n", rank, num_tasks);

#else
    rank = 0;
    num_tasks = 1;
#endif

    if (argc != 2) 
    {
        if (rank == 0) 
        {
            fprintf(stderr, "%s <n>\n", argv[0]);
            fprintf(stderr, "Program for parallel dense matrix-matrix multiplication\n");
            fprintf(stderr, "with 1D row partitioning\n");
            fprintf(stderr, "<n>: matrix dimension (an nxn dense matrix is created)\n");
#if USE_MPI
            MPI_Abort(MPI_COMM_WORLD, 1);
#else
            exit(1);
#endif
        }
    }

    int n;

    n = atoi(argv[1]);
    assert(n > 0);
    assert(n < 10000);

    /* ensure that n is a multiple of num_tasks */
    n = (n/num_tasks) * num_tasks;
    
    int n_p = (n/num_tasks);

    /* print new n to let user know n has been modified */
    if (rank == 0) 
    {
        fprintf(stderr, "n: %d, n_p: %d\n", n, n_p);
        fprintf(stderr, "Requires %3.6lf MB of memory per task\n", ((3*4.0*n_p)*n/1e6));
    }

    float *A, *B, *C, *tempBuf;
    
    A = (float *) malloc(n_p * n * sizeof(float));
    assert(A != 0);

    B = (float *) malloc(n_p * n * sizeof(float));
    assert(A != 0);
    
    C = (float *) malloc(n_p * n * sizeof(float));
    assert(C != 0);

    tempBuf = (float *) malloc(n_p * n * sizeof(float));
    assert(tempBuf != 0);

    /* linearized matrices in row-major storage */
    /* A[i][j] would be A[i*n+j] */

    int i, j;

    /* static initalization, so that we can verify output */
    /* using very simple initialization right now */
    /* this isn't a good check for parallel debugging */
#ifdef _OPENMP
#pragma omp parallel for private(i,j)
#endif
    for (i=0; i<n_p; i++) 
    {
        for (j=0; j<n; j++) 
        {
            A[i*n+j] = (rank+1);
            B[i*n+j] = 1;
            C[i*n+j] = 0;
        }
    }

#if USE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    double elt = 0.0;
    if (rank == 0) 
        elt = timer();

/*PARALLEL MATMUL CODE START##########################################################*/
#if USE_MPI

	// A[i][j] would be A[i*n+j] 
	int currentRow = rank*n_p;

    i = 0, j = 0;
    int k = 0, l = 0;
    // Comments assume n_p = 2, n = 4, num_tasks = 2
    for(l = 0; l<num_tasks; l++) // loop for each time an exchange between processes needs to occur
    {
    	for(i = 0; i < n_p; i++) // loop for each time
	    {
	    	for(j = 0; j < n_p; j++) // rank = 0, goes from 0 to 1 (2 iterations, 4 total)
		    {
		    	for(k = 0; k < n; k++) // rank = 0, goes from 0 to 3 (4 iterations, 16 total)
		    	{
		    		C[j*n+k] += A[j*n+i+currentRow]*B[i*n+k];
		    	}
		    }
	    }
	    // if(rank == 0)
    	// 	printMatrices(A, B, n, n_p);
    	// if(rank == 0)
		   //  printMatrix(C, n, n_p);

	    // If using more than one MPI processor we'll have to send messages
		if(num_tasks > 1)
		{
			// Receive your new B from the process above you, if you're the top process, send your B to the process below
			// Lowest rank
			if(rank == 0)
			{
				MPI_Sendrecv(B, n*n_p, MPI_FLOAT, num_tasks-1, l,
					tempBuf, n*n_p, MPI_FLOAT, rank+1, l,
					MPI_COMM_WORLD, &Stat);
			}
			// Highest rank
			else if( rank == num_tasks-1)
			{
				MPI_Sendrecv(B, n*n_p, MPI_FLOAT, rank-1, l,
					tempBuf, n*n_p, MPI_FLOAT, 0, l,
					MPI_COMM_WORLD, &Stat);
			}
			// Process number in the middle
			else
			{
				MPI_Sendrecv(B, n*n_p, MPI_FLOAT, rank-1, l,
					tempBuf, n*n_p, MPI_FLOAT, rank+1, l,
					MPI_COMM_WORLD, &Stat);
			}

			if(currentRow == n-n_p)
			{
				currentRow = 0;
			}
			else
			{
				currentRow += n_p;
			}
		}
		// If not, our work is already done and we can just leave the loop since it'll exit now anyway
		else
			break;
    }
/*PARALLEL MATMUL CODE END############################################################*/

/*NON-PARALLEL MATMUL CODE START######################################################*/
#else
    int k;
    for (i=0; i<n_p; i++) 
    {
        for (j=0; j<n; j++) 
        {
            float c_ij = 0;
            for (k=0; k<n; k++) 
            {
                c_ij += A[i*n+k]*B[k*n+j];
            }
            C[i*n+j] = c_ij;
        }
    }
#endif

    if (rank == 0) 
        elt = timer() - elt;

    /* Verify */
    int verify_failed = 0;
    for (i=0; i<n_p; i++) 
    {
        for (j=0; j<n; j++) 
        {
            if (C[i*n+j] != ((rank+1)*n))
                verify_failed = 1;
        }
    }

    if (verify_failed) 
    {
        fprintf(stderr, "ERROR: rank %d, verification failed, exiting!\n", rank);
#if USE_MPI
        MPI_Abort(MPI_COMM_WORLD, 2);
#else
        exit(2);
#endif
    }
    
    if (rank == 0) 
    {
        fprintf(stderr, "Time taken: %3.3lf s.\n", elt);
        fprintf(stderr, "Performance: %3.3lf GFlop/s\n", (2.0*n*n)*n/(elt*1e9));
    }

    /* free memory */
    free(A); free(B); free(C); free(tempBuf);

    /* Shut down MPI */
#if USE_MPI
    MPI_Finalize();
#endif

    return 0;
}

