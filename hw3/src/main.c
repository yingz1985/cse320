#include <stdio.h>
#include "sfmm.h"


int main(int argc, char const *argv[]) {
    sf_mem_init();

    //double* ptr = sf_malloc(sizeof(double));



    //*ptr = 320320320e-320;
    //sf_errno = 0;
    char *x = sf_malloc(32*sizeof(char)); //bs


    /* void *y = */ sf_malloc(10);

    x = sf_realloc(x, sizeof(char) * 100);
    sf_show_heap();
    sf_mem_fini();

    return EXIT_SUCCESS;
}
