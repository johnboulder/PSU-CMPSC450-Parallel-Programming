#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char **argv) {

    int x = 0;

#pragma omp parallel num_threads(6)
{

//#pragma omp critical
{
    x = x + 1;
}

}

    printf("Value of x is %d\n", x);

    return 0;
}
