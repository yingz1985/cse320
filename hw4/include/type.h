/*
    Header file contains struct type, and all of its necessary attributes

*/

#ifndef TYPE_H
#define TYPE_H

#include <stdint.h>
#include <stdlib.h>

typedef struct type {
    int id;            /* Determines bit in PRINTER_SET */
    char *name;        /* Name of the type. */
} TYPE;

#define MAX_TYPES 32
extern int CurrentNumberOfTypes;

TYPE TYPES_LIST[MAX_TYPES]; //list of user-defined types
PRINTER PRINTERS_LIST[MAX_PRINTERS];    //list of user-defined printers


#endif