#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <readline/readline.h>
#include <string.h>

#include "imprimer.h"
#include "debug.h"
#include "type.h"

void printHelpMenu();
void readToken();
int continueExecution = 1;


void commandLineInterface()
{
    char* line;
    while(continueExecution)
    {
        line = readline("imp>");
        if(strcmp(line,"help")==0)
        {

            printHelpMenu();
        }
        else if(strcmp(line,"quit")==0)
        {
                continueExecution = 0;
        }

        if(line)
            free(line);


    }
}

void printHelpMenu()
{
    printf("USAGE:\n\n\u2022"
        "Miscellaneous commands\n\t\u25e6help\n\t\u25e6quit\n"
        "\u2022Configuration commands\n\t\u25e6type file_type\n\t\u25e6printer printer_name file_type"
        "\n\t\u25e6conversion file_type1 file_type2 conversion_program[arg1 arg2...]\n"
        "\u2022Informational commands\n\t\u25e6printers\n\t\u25e6jobs\n"
        "\u2022Spooling commands\n\t\u25e6print file_name[printer1 printer 2...]"
        "\n\t\u25e6cancel job_number\n\t\u25e6pause job_number"
        "\n\t\u25e6resume job_number\n\t\u25e6disable printer_name"
        "\n\t\u25e6enable printer_name\n");


}


/*
 * "Imprimer" printer spooler.
 */

int main(int argc, char *argv[])
{


    CurrentNumberOfTypes = 0;
    char optval;
    while(optind < argc) {
	if((optval = getopt(argc, argv, "i:o:")) != -1) {
	    switch(optval) {
	    case '?':
		fprintf(stderr, "Usage: %s [-i <cmd_file>] [-o <out_file>]\n", argv[0]);
		exit(EXIT_FAILURE);
		break;
	    default:
		break;
	    }
	}
    }
    commandLineInterface();

    exit(EXIT_SUCCESS);
}
