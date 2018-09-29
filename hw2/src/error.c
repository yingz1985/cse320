/*
 * Error handling routines
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

int errors;
int warnings;
int dbflag = 1;

void fatal(char *fmt,...)
{
        va_list args;
        va_start(args,fmt);

        //fmt might be a formatted string
        //... is the arguments within the formatted string
        //since the original arguments a1 a2 a3 a4 a5 a6 are all of char* type
        // safely ASSUME that va_arg will have a string type
        fprintf(stderr, "\nFatal error: ");

        vfprintf(stderr,fmt,args);


        va_end(args);



        //fprintf(stderr, fmt, va_arg);
        fprintf(stderr, "\n");
        exit(1);
}

void error(char* fmt, ...)
{

        va_list args;
        va_start(args,fmt);

        fprintf(stderr, "\nError: ");
        vfprintf(stderr,fmt,args);
        fprintf(stderr, "\n");
        va_end(args);
        errors++;
}

void warning(char* fmt,...)
{
        va_list args;
        va_start(args,fmt);

        fprintf(stderr, "\nWarning: ");
        vfprintf(stderr,fmt,args);
        fprintf(stderr, "\n");
        va_end(args);
        warnings++;
}

void debug(char* fmt,...)
{
        va_list args;
        va_start(args,fmt);

        if(!dbflag) return;
        fprintf(stderr, "\nDebug: ");
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
}
