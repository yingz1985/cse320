#include <stdlib.h>

#include "hw1.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    //{0x2e736e64,800000,5,3,3,2}
    /*AUDIO_HEADER p ;//= {AUDIO_MAGIC,24,140780,3,8000,2};
    AUDIO_HEADER *hp = &p;

    read_header(hp);
    write_header(hp);


    int size = 32;
    char* string = malloc(size*sizeof(char));
    read_annotation(string,size);
    write_annotation(string,size);
    free(string);

    int* num = malloc(size*sizeof(int));
    int i = 0;
    for(i=0;i<140780;)0
    {
        read_frame(num,2,3);
        write_frame(num,2,3);
        i+=6;
    }
    free(num);

*/



    if(!validargs(argc, argv))
        {
           // printf("%lx",global_options);
            USAGE(*argv, EXIT_FAILURE);
        }
    debug("Options: 0x%lX", global_options);
    if(global_options & (0x1L << 63)) {
        USAGE(*argv, EXIT_SUCCESS);
    }
    //printf("%d",
    if(recode(argv)==0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
