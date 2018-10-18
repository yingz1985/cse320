#include <stdio.h>
#include "sfmm.h"


int main(int argc, char const *argv[]) {
    sf_mem_init();

    /*double* ptr = sf_malloc(sizeof(double));

    *ptr = 320320320e-320;

    //sf_show_heap();

    char* array = sf_malloc(4000*sizeof(char));
    *array = 'c';
    //printf("%f\n", *ptr);
    //sf_show_heap();

    sf_free(ptr);
    //sf_show_heap();
    sf_free(array);
    //sf_show_heap();*/

    void *u = sf_malloc(1); //32
    /* void *v = */ sf_malloc(40); //48
    void *w = sf_malloc(152); //160
    /* void *x = */ sf_malloc(536); //544
    void *y = sf_malloc(1); // 32
    /* void *z = */ sf_malloc(2072); //2080
    sf_malloc(3000);

    sf_free(u);
    sf_free(w);
    sf_free(y);

    sf_mem_fini();

    return EXIT_SUCCESS;
}
