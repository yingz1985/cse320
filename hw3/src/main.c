#include <stdio.h>
#include "sfmm.h"


int main(int argc, char const *argv[]) {
    sf_mem_init();

    double* ptr = sf_malloc(sizeof(double));

    *ptr = 320320320e-320;

    sf_mem_fini();

    return EXIT_SUCCESS;
}
