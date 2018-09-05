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
    AUDIO_HEADER p ;//= {AUDIO_MAGIC,24,140780,3,8000,2};
    AUDIO_HEADER *hp = &p;
    //int k =
    read_header(hp);
    write_header(hp);
    //write_header(hp);

    //printf("checked for header input %d\n",k);

    if(!validargs(argc, argv))
        {
           // printf("%lx",global_options);
            USAGE(*argv, EXIT_FAILURE);
        }
    debug("Options: 0x%lX", global_options);
    if(global_options & (0x1L << 63)) {
        USAGE(*argv, EXIT_SUCCESS);
    }

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
