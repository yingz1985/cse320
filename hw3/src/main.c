#include <stdio.h>
#include "sfmm.h"


int main(int argc, char const *argv[]) {
    sf_mem_init();

    double* ptr = sf_malloc(sizeof(double));

    *ptr = 320320320e-320;

    sf_show_heap();

    char* array = sf_malloc(4000*sizeof(char));
    //*array = 'c';
    //printf("%f\n", *ptr);
    sf_show_heap();

    sf_free(ptr);
    sf_free(array);

    sf_mem_fini();

    return EXIT_SUCCESS;
}
