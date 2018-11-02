#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <readline/readline.h>
#include <string.h>

#include "imprimer.h"
#include "debug.h"
#include "func.h"


int continueExecution = 1;
int CurrentNumberOfTypes;
int imp_num_printers;
PRINTER PRINTERS_LIST [MAX_PRINTERS];
char buffer[60];
char *bufferHead = buffer;
char *bufferEnd = buffer;
unsigned int sizeOfArg = 0;
PRINTER_SET set;
char ** argVar;
int argCount;
JOB head;//head of linked-list of jobs


CONVERSION conversionMatrix[32][32];
int conversionMap[32][32];





void commandLineInterface()
{
    char* line;
    while(continueExecution)
    {
        line = readline("imp>");
        processCommands(line);

        if(line)
        {
                free(line);

        }


    }
    //showTypes();

}
int parseLine(char*line)
{
    int pos = 0;
    while(*line != '\0' && *line !='\n')
    {

        int white = skipWhiteSpaces(line);
        line+=white;    // skip that many white spaces, could be zero if no white spaces
        char * arg = readArg(line);
        //debug("%s:arg",arg);
        if(pos==argCount)
        {
            debug("realloc");
            argCount+=argCount;
            argVar = realloc(argVar,argCount*sizeof(char*));
        }
        argVar[pos++] = arg;
        line+=(bufferEnd-bufferHead-1); //move to next word

    }
    for(int i = 0;i<pos;i++)
    {
        //printf("%s ",argVar[i]);
    }
    return pos;
}
int findPath(int file1ID,int file2ID)
{
    int prev[CurrentNumberOfTypes];
    int flag[CurrentNumberOfTypes];
    int queue[CurrentNumberOfTypes];
    initializeArray(prev,CurrentNumberOfTypes,-1);
    initializeArray(queue,CurrentNumberOfTypes,-1);
    initializeArray(flag,CurrentNumberOfTypes,0);

    int indexCursor = 0;
    int currentElementsQueued = 0;
    queue[currentElementsQueued++] = file1ID;
    int found = 0;
    //flag[file1ID] = 1;
    prev[file1ID] = file1ID;

    while(indexCursor<CurrentNumberOfTypes)
    {
        int v = queue[indexCursor++];
        int i = 0;
        while(i<CurrentNumberOfTypes && v!=-1)
        {
            //debug("%d%d %s",v,i,conversionMatrix[v][i].path);
            if(conversionMatrix[v][i].path != '\0' && flag[i]==0)
                //if there's an edge, and vertex is unvisited
            {
                //debug("not null %d%d",v,i);

                queue[currentElementsQueued++] = i;
                prev[i] = v;
                flag[i] = 1;//visited the node
                if(i==file2ID)
                {
                    found = 1;
                    i= CurrentNumberOfTypes;
                    indexCursor = CurrentNumberOfTypes;
                    break;
                }
            }
            i++;
        }
        flag[v]=1;
    }
    if(found)
    {
        int vertex = file2ID;
        while(prev[vertex]!=vertex)
        {
            printf("%s ",TYPES_LIST[vertex].name);
            vertex = prev[vertex];
        }
        printf("%s \n",TYPES_LIST[vertex].name);
    }

    return found;






}
void initializeArray(int * array,int size,int val)
{
    for (int i = 0;i<size;i++)
    {
        array[i] = val;
    }
}
void processCommands(char*line)
{
        int numArgs = parseLine(line);
        //debug("numArgs: %d",numArgs);
        //char* arg = readArg(line);

        if(strcmp(argVar[0],"help")==0 && numArgs==1)
        {

            printHelpMenu();
        }
        else if(strcmp(argVar[0],"type")==0)
        {

            if(numArgs<2)
                 error("requires type argument");
            else if (numArgs>2)
                error("extra arguments not needed was entered");
            else
            {

                addType(argVar[1]);

            }

        }
        else if(strcmp(argVar[0],"printer")==0)
        {
            processCommandPrinter(numArgs);
        }
        else if(strcmp(argVar[0],"conversion")==0)
        {
            processCommandConversion(numArgs);
        }
        else if(strcmp(argVar[0],"printers")==0)
        {
            showPrinters();
        }
        else if(strcmp(argVar[0],"jobs")==0)
        {
            showJobs();
        }
        else if(strcmp(argVar[0],"print")==0)
        {
            //create job to be queued


        }

        else if(strcmp(argVar[0],"quit")==0 &&  numArgs==1)
        {
                continueExecution = 0;
        }


    //free(argVar[0]);

}

void processCommandConversion(int numArgs)
{
    if(numArgs>=4)
    {


            int find1 = findFileType(argVar[1]);
            int find2 = findFileType(argVar[2]);
            if(find1&&find2)
            {

                char** argForConvProg = NULL;
                if(numArgs>4)
                    argForConvProg = argVar+3;
                int index1 = findFileIndex(argVar[1]);
                int index2 = findFileIndex(argVar[2]);
                CONVERSION conProg = {
                    index1,index2,
                    argVar[3],argForConvProg
                };
                //debug("made conversion struct %d %d",index1,index2);

                conversionMatrix[index1][index2] = conProg; //matrix of conversion programs



            }
            else
            {
                //debug("%s:%d %s:%d",argVar[1],find1,argVar[2],find2);
                error("one or more file type(s) are not supported");
            }

    }
    else
    {
        //debug("%s: %s",argVar[1],argVar[2]);
        error("not enough arguments were supplied");
    }


}
void processCommandPrinter(int numArgs)
{

    if(numArgs==3)
    {
        addPrinter(argVar[1],argVar[2]);
    }
    else if(numArgs>3)
    {
        error("extra arguments not needed was inputted ");
    }
    else
    {
        error("must input both printer name and file type ");
    }



}
void addPrinter(char* printerName,char *fileName)
{
    int found = findPrinter(printerName);
    if(!found)
    {
        int foundFile = findFileType(fileName);
        if(foundFile)
        {

            PRINTER newPrinter = {imp_num_printers,printerName,fileName,0,0,NULL};
            PRINTERS_LIST[imp_num_printers++] = newPrinter;
            set|= (0x1<<newPrinter.id);
            debug("set of printers %x\nprinter name:%s",set,newPrinter.name);
        }
        else
        {
            error("file type %s is not an accepted type",fileName);
        }
    }
    else
    {
        error("printer %s has already been declared",printerName);
    }



}
int findPrinter(char* name)
{
    int i;
    for(i= 0;i<imp_num_printers;i++)
    {
        if(strcmp(PRINTERS_LIST[i].name,name)==0 )
        {
            printf("printer name %s i = %d",PRINTERS_LIST[i].name,i);
            return 1;

        }
    }
    return 0;
}

void addType(char *name)
{

    int found = findFileType(name);
    if(!found)
    {
        TYPE newType = {CurrentNumberOfTypes,name};
        TYPES_LIST[CurrentNumberOfTypes++] = newType;
        //debug("type name %s1",name);
    }
    else
    {
        error("type %s has already been declared",name);
    }
}
int findFileType(char*name)
{
    int i;
    for(i= 0;i<CurrentNumberOfTypes;i++)
    {

        if(strcmp(TYPES_LIST[i].name,name)==0 )
        {
            return 1;
        }
    }
    return 0;
}
int findFileIndex(char*name)
{
    int i;
    for(i= 0;i<CurrentNumberOfTypes;i++)
    {
        if(strcmp(TYPES_LIST[i].name,name)==0 )
        {
            return i;

        }
    }
    return -1;
}
void printConversionprograms()
{
    for(int i = 0 ;i<32;i++)
    {
        for(int k = 0 ;k<32;k++)
        {
            if(conversionMatrix[i][k].path != '\0')
                debug("%s ",conversionMatrix[i][k].path);
        }

    }
}


void showTypes()
{
    int i;
    for(i= 0;i<CurrentNumberOfTypes;i++)
    {
        debug("%s\n",TYPES_LIST[i].name);
    }
}
void showPrinters()
{
    char * buf = malloc(sizeof(char*)*100);
    //char *imp_format_printer_status(PRINTER *printer, char *buf, size_t size);
    for(int i = 0;i<imp_num_printers;i++)
    {
        printf("%s\n",imp_format_printer_status(&PRINTERS_LIST[i], buf, 100));
    }
    free(buf);
}
void showJobs()
{
    JOB* currentJob = &head;
    char * buf = malloc(sizeof(char*)*100);
    while(currentJob->other_info!=&head)
    {
        printf("%s\n",imp_format_job_status(currentJob, buf, 100));
        currentJob = currentJob->other_info;
    }
    free(buf);
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

void testBFS();
/*
 * "Imprimer" printer spooler.
 */

int main(int argc, char *argv[])
{
    argCount = 5;

    argVar = calloc(argCount,argCount*sizeof(char(*)));
    //conversionMatrix = calloc(MAX_TYPES*MAX_TYPES,MAX_TYPES*MAX_TYPES*sizeof(CONVERSION));
    memset(conversionMatrix,0,MAX_TYPES*MAX_TYPES*sizeof(CONVERSION));
    FILE* infile;
    CurrentNumberOfTypes = 0;
    imp_num_printers = 0;
    char optval;
    int readFromFile = 0;
    while(optind < argc)
    {
    	if((optval = getopt(argc, argv, "i:o:")) != -1) {
    	    switch(optval) {
            case 'i':
                infile = fopen(optarg,"r");
                if(infile==NULL)
                    error("failed to open file");
                readFromFile = 1;

                break;
    	    case '?':
    		  fprintf(stderr, "Usage: %s [-i <cmd_file>] [-o <out_file>]\n", argv[0]);
    		  exit(EXIT_FAILURE);
    		  break;
    	    default:
    		break;
    	    }
    	}
    }

    if(readFromFile)
    {
        char* line = (char*)calloc(60,sizeof(char)*60);
        while(fgets(line,60,infile)!=NULL)
        {

            processCommands(line);
        }
        //printConversionprograms();

    }
    //showTypes();
    commandLineInterface();
    testBFS();

    exit(EXIT_SUCCESS);
}
void testBFS()
{
    findPath(0,1);
    findPath(1,4);
    findPath(8,5);
}


char * argString(char*buf,int size)
{
    char* string,*temp;
    if((string = (char*)malloc(size))==NULL)
    {
        error("memory not allocated for string");
        return NULL;
    }
    else
    {
        temp = string;
        //debug("string size:%d",size);
        while(size-- >0)
        {
            if(*buf!=13)
                *temp++ = *buf++;
            //debug("%c %d",*(temp-1),*(temp-1));

        }
        return string;
    }


}
void clearBuffer()
{
    bufferHead=buffer;
    bufferEnd=buffer;

}
int lengthOf(char* string)
{
    int count= 0;
    while((*(string+count))!='\n' && (*(string+count))!='\0')
    {
        count++;
    }
    return count;
}

int isWhiteSpace(char i)
{
    if(i==32|| i== '\n' || i=='\t'||i=='\0'||i==' ')
        return 1;
    return 0;
}
int skipWhiteSpaces(char*line)
{
    int space =0;
    while(isWhiteSpace(*(line+space)))
    {
        space ++;
    }
    return space;
}

void readUntilWhitespace(char* line)
{
    clearBuffer();
    unsigned int size = 0;
    //line+=skipWhiteSpaces(line);
    while(!isWhiteSpace(*(line+size)))
    {

        //debug("currentChar: %c size:%u\n",*(line+size),size);
        *bufferEnd++ = *(line+size);
        size++;
    }
    if(bufferEnd != buffer)
        *bufferEnd++ = '\0';

}
char* readArg(char* line)   //returns string of only the argument
{
    readUntilWhitespace(line);
    //debug("read until white space\n");
    sizeOfArg = bufferEnd-bufferHead;

    return argString(buffer,sizeOfArg);
}

