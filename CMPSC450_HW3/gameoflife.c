#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#define USE_MPI 1

#if USE_MPI
#include <mpi.h>
#endif

static double timer() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) (tp.tv_sec) + 1e-6 * tp.tv_usec);
}

void printGrid(int *grid, int m, int m_p)
{
    int i, j;

    if(m*m_p < 30)
    {
        for (i=0; i<m_p; i++) 
        {
            for (j=0; j<m; j++) 
            {
                fprintf(stderr, "%d", grid[i*m + j]);

                if(j  == m-1)
                    fprintf(stderr, "\n");
                else
                    fprintf(stderr, " ");
            }
        }

        fprintf(stderr, "\n");
        fprintf(stderr, "\n");
    }
}

int main(int argc, char **argv) {

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

    if (argc != 3) {
        if (rank == 0) {
            fprintf(stderr, "%s <m> <k>\n", argv[0]);
            fprintf(stderr, "Program for parallel Game of Life\n");
            fprintf(stderr, "with 1D grid partitioning\n");
            fprintf(stderr, "<m>: grid djmension (an mxm grid is created)\n");
            fprintf(stderr, "<k>: number of time steps\n");
            fprintf(stderr, "(initial pattern specified inside code)\n");
#if USE_MPI
            MPI_Abort(MPI_COMM_WORLD, 1);
#else
            exit(1);
#endif
        }
    }

    int m, k;

    m = atoi(argv[1]);
    assert(m > 2);
    assert(m <= 10000);

    k = atoi(argv[2]);
    assert(k > 0);
    assert(k <= 1000);

    /* ensure that m is a multjple of num_tasks */
    m = (m/num_tasks) * num_tasks;
    
    int m_p = (m/num_tasks);

    /* print new m to let user know n has been modified */
    if (rank == 0) {
        fprintf(stderr, "Using m: %d, m_p: %d, k: %d\n", m, m_p, k);
        fprintf(stderr, "Requires %3.6lf MB of memory per task\n", 
                ((2*4.0*m_p)*m/1e6));
    }

    /* Linearizing 2D grids to 1D using row-major ordering */
    /* grid[i][j] would be grid[i*n+j] */
    int *grid_current;
    int *grid_next;
    int *row_ghost_top;
    int *row_ghost_bot;
    
    grid_current = (int *) malloc(m_p * m * sizeof(int));
    assert(grid_current != 0);

    grid_next = (int *) malloc(m_p * m * sizeof(int));
    assert(grid_next != 0);

    row_ghost_top = (int *) malloc(m * sizeof(int));
    assert(row_ghost_top != 0);

    row_ghost_bot = (int *) malloc(m * sizeof(int));
    assert(row_ghost_bot != 0);

    int i, j, t;

    /* static initalization, so that we can verify output */
    /* using very sjmple initialization right now */
    /* this isn't a good check for parallel debugging */
#ifdef _OPENMP
#pragma omp parallel for private(i,j)
#endif
    for (i=0; i<m_p; i++) {
        for (j=0; j<m; j++) {
            grid_current[i*m+j] = 0;
            grid_next[i*m+j] = 0;
        }
    }

    /* initializing some cells in the middle with a glider*/
    assert((m*m_p/2 + m/2 + 3) < m_p*m);
    grid_current[m*m_p/2 + m/2 + 1] = 1;
    grid_current[m*m_p/2 + m/2 + 2] = 1;
    grid_current[m*m_p/2 + m/2 + 3] = 1;
    grid_current[m*m_p/2 - m + m/2 + 3] = 1;
    grid_current[m*m_p/2 - 2*m + m/2 + 2] = 1;


if(rank == 0 || num_tasks == 1)
{
    printGrid(grid_current, m, m_p);
}

#if USE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    double elt = 0.0;
    if (rank == 0) 
        elt = timer();

//#if USE_MPI
    /*PARALLEL MATMUL CODE START##########################################################*/

    /* Parallel code goes here */
    int l = 0, rankAbove = rank - 1, rankBelow = rank + 1, ul, left, bl, b, ur, r, br;
    int *tempBuf = (int *) malloc(m * sizeof(int));
    assert(tempBuf != 0);
    // Determine what rank you'll be passing messages to
    if(rank == 0)
    {
        rankAbove = rank + 1;
        rankBelow = num_tasks - 1;
    }
    else if(rank == num_tasks - 1)
    {
        rankAbove = rank - 1;
        rankBelow = 0;
    }

    // Each process has a part of the matrix that is m x (m_p)
    // loop for the number of time steps that must be sjmulated
    for(i = 0; i<k; i++)
    {
       if(num_tasks > 1)
       {
            // Copy the bottom row of grid_current
            memcpy(row_ghost_bot, grid_current+ m*(m_p-1), sizeof(int) * m);
            // Copy the top row of grid_current
            memcpy(row_ghost_top, grid_current, sizeof(int) * m);

            // Send bottom row to rank below, receive top ghost row from rank above
            MPI_Sendrecv(row_ghost_bot, m, MPI_INT, rankBelow, i,
                tempBuf, m, MPI_INT, rankAbove, i,
                MPI_COMM_WORLD, &Stat);

            // Send top row to rank above, receive bottom ghost row from rank below
            MPI_Sendrecv(row_ghost_top, m, MPI_INT, rankAbove, i, 
                row_ghost_bot, m, MPI_INT, rankBelow, i,
                MPI_COMM_WORLD, &Stat);

            // Copy the tempBuf into row_ghost_top
            memcpy(row_ghost_top, tempBuf, sizeof(int) * m);
       }

        // loop for the rows to scan
        for(j = 0; j<m_p; j++)
        {
            // loop for the columns to scan
            for(l = 0; l<m; l++)
            {
                int nsum = 0;
                // ul, l, bl, t, b, ur, r, br
                t = (j-1)*m + l;
                ur = (j-1)*m + l+1;
                r = j*m + l+1;
                br = (j+1)*m + l+1;
                b = (j+1)*m + l;
                bl = (j+1)*m + l-1;
                left = j*m + l-1;
                ul = (j-1)*m + l-1;
                // Left side of grid
                if(l == 0)
                {
                    left = j*m + m-1;
                    nsum += grid_current[r] + grid_current[left];
                    // Top
                    if(j == 0)
                    {
                        bl = (j+1)*m + m-1;
                        // bl comes from opposite side of grid
                        nsum += grid_current[br] + grid_current[b] + grid_current[bl];
                        
                        if(num_tasks>1)
                        {
                            // t, ur, ul come from the ghost row
                            t = l;
                            ur = l+1;
                            ul = m-1;

                            nsum += row_ghost_top[t] + row_ghost_top[ur] + row_ghost_top[ul];
                        }
                        else
                        {
                            t = (m*(m_p-1));
                            ur = (m*(m_p-1)+1);
                            ul = (m*m_p-1);

                            nsum += grid_current[t] + grid_current[ur] + grid_current[ul];
                        }
                    }
                    // Bottom
                    else if(j == m_p-1)
                    {

                        ul = (j-1)*m + m-1;
                        nsum += grid_current[ur] + grid_current[t] + grid_current[ul];

                        // ARE THERE MULTIPLE MPI TASKS
                        if(num_tasks>1)
                        {
                            b = l;
                            br = l+1;
                            bl = m-1;

                            nsum += row_ghost_bot[b] + row_ghost_bot[br] + row_ghost_bot[bl];
                        }
                        else
                        {
                            b = l;
                            br = l+1;
                            bl = m-1;

                            nsum += grid_current[b] + grid_current[br] + grid_current[bl];
                        }
                    }
                    // Middle
                    else
                    {
                        ul = (j-1)*m + m-1;
                        bl = (j+1)*m + m-1;
                        nsum += grid_current[ur] + grid_current[t] + grid_current[ul] + grid_current[bl] + grid_current[b] + grid_current[br];
                    }
                }
                // Right side of grid
                else if(l == (m-1))
                {
                    r = j*m;
                    nsum += grid_current[r] + grid_current[left];

                    // Top
                    if(j == 0)
                    {
                        br = (j+1)*m;
                        // br comes from opposite side of grid
                        nsum += grid_current[br] + grid_current[b] + grid_current[bl];

                        // ARE THERE MULTIPLE MPI TASKS
                        if(num_tasks>1)
                        {
                            // t, ur, ul come from the ghost row
                            t = l;
                            ur = 0;
                            ul = l-1;

                            nsum += row_ghost_top[t] + row_ghost_top[ur] + row_ghost_top[ul];
                        }
                        else
                        {
                            t = (m*(m_p-1)) + l;
                            ur = (m*(m_p-1));
                            ul = (m*(m_p-1)) + l-1;

                            nsum += grid_current[t] + grid_current[ur] + grid_current[ul];
                        }
                        
                    }
                    // Bottom
                    else if(j == m_p-1)
                    {

                        ur = (j-1)*m;
                        nsum += grid_current[ur] + grid_current[t] + grid_current[ul];


                        if(num_tasks>1)
                        {
                            b = l;
                            br = 0;
                            bl = l-1;

                            nsum += row_ghost_bot[b] + row_ghost_bot[br] + row_ghost_bot[bl];
                        }
                        else
                        {
                            b = l;
                            br = l+1;
                            bl = l-1;

                            nsum += grid_current[b] + grid_current[br] + grid_current[bl];
                        }
                    }
                    // Middle
                    else
                    {
                        ur = (j-1)*m;
                        br = (j+1)*m;
                        nsum += grid_current[ur] + grid_current[t] + grid_current[ul] + grid_current[bl] + grid_current[b] + grid_current[br];
                    }
                }
                // we're somewhere in the middle
                else 
                {
                    // Top Middle
                    if(j == 0)
                    {
                        if(num_tasks>1)
                        {
                            t = l;
                            ur = l+1;
                            ul = l-1;

                            nsum =  row_ghost_top[t]  + 
                                    row_ghost_top[ur] + 
                                    grid_current[r]  + 
                                    grid_current[br] + 
                                    grid_current[b]  + 
                                    grid_current[bl] + 
                                    grid_current[left]  + 
                                    row_ghost_top[ul];
                        }
                        else
                        {
                            t = (m*(m_p-1)) + l;
                            ur = (m*(m_p-1)) + l + 1;
                            ul = (m*(m_p-1)) + l - 1;

                            nsum =  grid_current[t]  + 
                                    grid_current[ur] + 
                                    grid_current[r]  + 
                                    grid_current[br] + 
                                    grid_current[b]  + 
                                    grid_current[bl] + 
                                    grid_current[left]  + 
                                    grid_current[ul];
                        }
                        // Gotta change ul, t, and ur
                    }
                    // Bottom Middle
                    else if(j == m_p-1)
                    {
                        if(num_tasks>1)
                        {
                            b = l;
                            br = l+1;
                            bl = l-1;

                            nsum =  grid_current[t]  + 
                                    grid_current[ur] + 
                                    grid_current[r]  + 
                                    row_ghost_bot[br] + 
                                    row_ghost_bot[b]  + 
                                    row_ghost_bot[bl] + 
                                    grid_current[left]  + 
                                    grid_current[ul];
                        }
                        else
                        {
                            b = l;
                            br = l+1;
                            bl = l-1;

                            nsum =  grid_current[t]  + 
                                    grid_current[ur] + 
                                    grid_current[r]  + 
                                    grid_current[br] + 
                                    grid_current[b]  + 
                                    grid_current[bl] + 
                                    grid_current[left]  + 
                                    grid_current[ul];
                        }
                        // Gotta change bl, b, and br
                    
                    }
                    else
                    {
                        // t, ur, r, br, b, bl, l, ul
                        nsum =  grid_current[t]  + 
                                grid_current[ur] + 
                                grid_current[r]  + 
                                grid_current[br] + 
                                grid_current[b]  + 
                                grid_current[bl] + 
                                grid_current[left]  + 
                                grid_current[ul];

                        // if(l == 2 && j == 3)
                        // {
                        //     fprintf(stderr, "nsum: %d\n", nsum);
                        //     fprintf(stderr, "%d ", grid_current[ul]);
                        //     fprintf(stderr, "%d ", grid_current[t]);
                        //     fprintf(stderr, "%d \n", grid_current[ur]);
                        //     fprintf(stderr, "%d ", grid_current[left]);
                        //     fprintf(stderr, "l ");
                        //     fprintf(stderr, "%d \n", grid_current[r]);
                        //     fprintf(stderr, "%d ", grid_current[bl]);
                        //     fprintf(stderr, "%d ", grid_current[b]);
                        //     fprintf(stderr, "%d \n\n", grid_current[br]);
                        // }
                    }
                }

                /*
                Rules of the game
                1. Any live cell with fewer than two live neighbours dies, as if caused by under-population.
                2. Any live cell with two or three live neighbours lives on to the next generation.
                3. Any live cell with more than three live neighbours dies, as if by over-population.
                4. Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
                */
                switch(nsum)
                {

                    case 3:
                      grid_next[j*m + l] = 1;
                      break;

                    case 2:
                      grid_next[j*m + l] = grid_current[j*m + l];
                      break;

                    default:
                      grid_next[j*m + l] = 0;
                }
            }
        }

        int *grid_tmp  = grid_next;
        grid_next = grid_current;
        grid_current = grid_tmp;

        // #ifdef _OPENMP
        // #pragma omp parallel for private(i,j)
        // #endif
        // for (i=0; i<m_p; i++) {
        //     for (j=0; j<m; j++) {
        //         grid_next[i*m+j] = 0;
        //     }
        // }
    }

    if (rank == 0 && num_tasks == 1)
    {
        FILE *f = fopen("verification.data", "w");
        fwrite(grid_current, sizeof(int), m*m_p, f);
        fclose(f);
    }

    /*PARALLEL MATMUL CODE END############################################################*/
// #else
//     /* serial code */
//     /* considering only internal cells */
//     for (t=0; t<k; t++) 
//     {
//         for (i=0; i<m-1; i++) {
//             for (j=0; j<m-1; j++) {
//                 /* avoiding conditionals inside inner loop */
//                 int prev_state = grid_current[i*m+j];
//                 // 
//                 int num_alive  = 
//                                 grid_current[(i  )*m+j-1] + 
//                                 grid_current[(i  )*m+j+1] + 
//                                 grid_current[(i-1)*m+j-1] + 
//                                 grid_current[(i-1)*m+j  ] + 
//                                 grid_current[(i-1)*m+j+1] + 
//                                 grid_current[(i+1)*m+j-1] + 
//                                 grid_current[(i+1)*m+j  ] + 
//                                 grid_current[(i+1)*m+j+1];

//                 grid_next[i*m+j] = prev_state * ((num_alive == 2) + (num_alive == 3)) + (1 - prev_state) * (num_alive == 3);
//             }
//         }
//         /* swap current and next */
//         int *grid_tmp  = grid_next;
//         grid_next = grid_current;
//         grid_current = grid_tmp;
//     }
// #endif

    if (rank == 0) 
        elt = timer() - elt;

    if(rank == 0)
    {
        printGrid(grid_current, m, m_p);
    }

    /* Verify */
    // int *verif_array = malloc(sizeof(int) * m*m_p);
    int verify_failed = 0;

    // FILE *ifp = fopen("verification.data", "r");
    // if(ifp == NULL)
    // {
    //     verify_failed = 1;
    //     free(verif_array);
    // }
    // else
    // {
    //     fseek(ifp, rank*m, 0);
    //     fread(verif_array, sizeof(int), m*m_p, ifp);
    //     for (i=0; i<m_p; i++) 
    //     {
    //         for (j=0; j<m; j++) 
    //         {
    //             /* Add verification code here */
    //             if(grid_current[i*m + j] != verif_array[i*m+j])
    //             {
    //                 verify_failed = 1;
    //                 break;
    //             }
    //         }
    //         if(verify_failed)
    //             break;
    //     }
    //     free(verif_array);
    // }

    if (verify_failed) {
        fprintf(stderr, "ERROR: rank %d, verification failed, exiting!\n", rank);
#if USE_MPI
        MPI_Abort(MPI_COMM_WORLD, 2);
#else
        exit(2);
#endif
    }

    if (rank == 0) 
    {
        fprintf(stderr, "time taken: %3.3lf s.\n", elt);
        fprintf(stderr, "Performance: %3.3lf billion cell updates/s\n", (1.0*m*m)*k/(elt*1e9));
    }

    /* free memory */
    free(grid_current); free(grid_next);

    /* Shut down MPI */
#if USE_MPI
    MPI_Finalize();
#endif

    return 0;
}
