#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>

static double timer() 
{
    
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) (tp.tv_sec) + 1e-6 * tp.tv_usec);

}

int my_Allgather(int *sendbuf, int n, int nprocs, int *recvbuf, int rank) 
{

    int i;
    for (i=0; i<n*nprocs; i++) 
    {
        recvbuf[i] = 0;
    }

    memcpy( &recvbuf[rank*n], sendbuf, n * sizeof(int));
    
    /* recursive doubling-based code goes here */
    int maxLevel = (int) log2((double)nprocs);
    int rank2;
    int power;
    int start = rank;
    int start2;  
    MPI_Status stat;

    for (i=0; i < maxLevel; i++)
    {
        power = (int) pow(2.0, i);

        // Emulate signed addition by some 2^i
        rank2 = rank ^ (1 << i);
        
        MPI_Sendrecv(&start, 1, MPI_INT, rank2, 0, &start2, 1, MPI_INT, rank2, 0, MPI_COMM_WORLD, &stat);
        MPI_Sendrecv(&recvbuf[start*n], n*power, MPI_INT, rank2, 1, &recvbuf[start2*n], n*power, MPI_INT, rank2, 1, MPI_COMM_WORLD, &stat);

        if (start2 < start) 
            start = start2;
        
        MPI_Barrier(MPI_COMM_WORLD);
    }
    return 0;
} 

int main(int argc, char **argv) 
{

    int rank, nprocs;
    int i;

    /* Initialize MPI Environment */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* change the input size here */
    int n;
    if (argc == 2) 
    {
        n = atoi(argv[1]);
    } 
    else 
    {
        n = 4;
    }
    if (rank == 0) 
    {
        fprintf(stderr, "n: %d\n", n);
    }

    int *sendbuf;
    int *recvbuf1;
    int *recvbuf2;
    
    sendbuf  = (int *) malloc(n*sizeof(int));
    assert(sendbuf != 0);

    for (i=0; i<n; i++) {
        sendbuf[i] = (rank+1);
    }

    recvbuf1 = (int *) malloc(n*nprocs*sizeof(int));
    assert(recvbuf1 != 0);

    recvbuf2 = (int *) malloc(n*nprocs*sizeof(int));
    assert(recvbuf2 != 0);

    double allGatherStd = 0.0;
    if (rank == 0) allGatherStd = timer();
    MPI_Allgather(sendbuf, n, MPI_INT, recvbuf1, n, MPI_INT, MPI_COMM_WORLD);
    if (rank == 0) allGatherStd = timer() - allGatherStd;

    double allGatherCustom = 0.0;
    if (rank == 0) allGatherCustom = timer();
    my_Allgather(sendbuf, n, nprocs, recvbuf2, rank);
    if (rank == 0) allGatherCustom = timer() - allGatherCustom;
    
    /* verify that my_Allgather works correctly */
    for (i=0; i<n*nprocs; i++) {
        assert(recvbuf1[i] == recvbuf2[i]);
    }

    free(sendbuf); free(recvbuf1); free(recvbuf2);

    /* Terminate MPI environment */
    MPI_Finalize();

    // Print out times
    if (rank == 0) {
        printf("Elapsed time (standard) : %9.6lf s\n", allGatherStd);
        printf("Elapsed time (custom)   : %9.6lf s\n", allGatherCustom);
    }

    return 0;
}
