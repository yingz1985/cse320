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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>


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

CONVERSION path[32];
JOB* jobQueue;
int jobsCreated;
char* showBuf;


CONVERSION conversionMatrix[32][32];





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
            //debug("realloc");
            argCount+=argCount;
            argVar = realloc(argVar,argCount*sizeof(char*));
        }
        argVar[pos++] = arg;
        line+=(bufferEnd-bufferHead-1); //move to next word

    }

    return pos;
}
int findPath(int file1ID,int file2ID)
{

    if(file1ID==file2ID)
    {

        return 0;

    }
    int prev[CurrentNumberOfTypes];
    int flag[CurrentNumberOfTypes];
    int queue[CurrentNumberOfTypes];
    initializeArray(prev,CurrentNumberOfTypes,-1);
    initializeArray(queue,CurrentNumberOfTypes,-1);
    initializeArray(flag,CurrentNumberOfTypes,0);

    int indexCursor = 0;
    int currentElementsQueued = 0;
    queue[currentElementsQueued++] = file1ID;
    int found = -1;
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
        int i = 0;
        while(prev[vertex]!=vertex)
        {
            path[i] = conversionMatrix[prev[vertex]][vertex];

            vertex = prev[vertex];
            i++;
        }
        //printf("%s \n",TYPES_LIST[vertex].name);
        return i;
    }

    return found;






}
JOB* createJob(char* file_name,char*file_type,PRINTER_SET set,PRINTER *chosen)
{
    struct timeval currentTime;

    gettimeofday(&currentTime,NULL);


    JOB* job = (JOB*)malloc(sizeof(JOB));
    JOB j = {jobsCreated++,QUEUED,0,file_name,file_type,set,chosen,currentTime,currentTime,NULL};
    *job = j;

    if(jobQueue==NULL)
    {
        jobQueue = job;

        //printf("%s\n",imp_format_job_status(jobQueue, showBuf, 100));
    }
    else
    {
        JOB* currentJob = jobQueue;//first job
        //printf("%s\n",imp_format_job_status(jobQueue, showBuf, 100));
        while(currentJob->other_info!=NULL)
        {
            //while job queue has next job in queue, keep going
            currentJob = ((JOB*)currentJob->other_info);
        }
        //currentJob.other_info = malloc(sizeof(JOB*));;
        currentJob->other_info = job;  //add job to end of queue
       // printf("%s\n",imp_format_job_status((JOB*)(currentJob->other_info), showBuf, 100));


    }
    //printf("%s\n",imp_format_job_status(&job, showBuf, 100));
    return job;


}

void showJobs()
{
    JOB* currentJob = jobQueue;
    //char * buf = malloc(sizeof(char)*100);
    while(currentJob!=NULL)//while has other info
    {
        printf("%s\n",imp_format_job_status(currentJob, showBuf, 100));
        currentJob = (JOB*)currentJob->other_info;
    }
   // free(buf);
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

            PRINTER_SET eligiblePrinters = 0;
            if(numArgs>2)
            {
                    eligiblePrinters = fillPrinterSet(numArgs);

            }
                //if optional printer names are specified
            else
            {
                    eligiblePrinters = set;
            }

            int sourceType = retrieveFileType(argVar[1]);
            if(sourceType!=-1)
            {
                PRINTER* print = findPrinterToPrint(eligiblePrinters,sourceType);
                printf("create job for %s\n",print->name);
                JOB* jobo = createJob(argVar[1],findIndexOfDOT(argVar[1]),eligiblePrinters,print);
                makeConversions(jobo);

                //did not find printer to print currently, but will queue the job
                //showJobs();

            }

            //create job to be queued


        }

        else if(strcmp(argVar[0],"quit")==0 &&  numArgs==1)
        {
                free(argVar[0]);
                continueExecution = 0;
        }


    //free(argVar[0]);

}
void makeConversions(JOB * job)
{
    //assuming that a valid printer was found and path was also found accordingly
    //assuming size is not zero
    //this is the method that makes the processes **
    int conversionsNeeded = *(int*)job->chosen_printer->other_info;

    int* conversionPipes = (int*)malloc(sizeof(int)*conversionsNeeded*2);
    //[conversionsNeeded][2];// a list of pipes connecting the processes
    CONVERSION* localCopy = (CONVERSION*)malloc(conversionsNeeded*sizeof(CONVERSION));
    //[conversionsNeeded];
    memcpy(localCopy,path,conversionsNeeded*sizeof(CONVERSION));
    //make a local copy of the conversion pipes

    pid_t pid;
    int infile = open(job->file_name,O_RDONLY);
    int * processList = (int*)malloc(conversionsNeeded*sizeof(int));
    int printerDesc = imp_connect_to_printer(job->chosen_printer, 0x0);
    fprintf(stderr,"connect to printer %d infile %d\n",printerDesc,infile);
    fflush(stderr);
    //infile = file descripter for first process to convert
    int status = 0;
    if((pid=fork())==0)
    {
        //master process
        printf("created master process\n");
        printf("conversions %d\n",conversionsNeeded);

        forker(conversionsNeeded,0,infile,1,conversionPipes,localCopy,processList);
        printf("forked\n");
        for(int i = 0;i<conversionsNeeded;i++)
        {

            waitpid(processList[i],&status,0);
            printf("child %d exited with status %d\n",processList[i],status);
        }
        printf("exiting master process");
        _exit(EXIT_SUCCESS);

    }
    else
    {
        //main- do nothing
        printf("main\n");
        setpgid(pid,pid);
        //waitpid(pid,&status,0);
    }



}

void forker(int nConversions,int index,int fileDesc,int outputDesc,int* pipes,CONVERSION* conversions,int* pidList)
{
    pid_t pid;
    if(nConversions>0)
    {
        if((pid = fork()) == 0)
        {

            //printf("\nhello\n");
            //child stuff
            //pid_t parentID = getppid();
            //pid_t parentGroup = getpgrp();
            //child inherits the process group of the parent
            pipe(pipes+2*index);

            if(!index)
            {//index == 0; first child process has to get input from fileDesc

                dup2(fileDesc,0);
                //close(fileDesc);
                //close(pipes[index][1]);
            }

            else
            {

                dup2(pipes[2*index-1],0);  //take input from previous output
                //close(pipes[index-1][1]);
                //close(pipes[index][1]);

                //read from output of previous pipe
            }

            if(nConversions==1)
            {
                //output to printer descriptor

                dup2(outputDesc,1);
                //for now output to stdout

            }

            else
            {

                dup2(pipes[2*index+1],1);
                //output to write end of pipe wait to be read


            }

            //for(int i = 0;i<2*index;i++)
            {
                //close(pipes[i]);
            }
            /*char** arg = NULL;
            arg[0] = conversions[index].path;
            if(conversions[index].argVar)
                arg = conversions[index].argVar;
*/
            int execute = execvp(conversions[index].path,conversions[index].argVar);
            //if execvp returned it must've failed
            fprintf(stderr,"finished executing");
            fflush(stderr);
            if(execute==-1)
            {
                exit(1);
            }
            exit(0);


        }
        else if(pid<0)
        {
            perror("fork errored");
        }
        else
        {
            //pid >0 parent process
            printf("tried to create new process\n");
            //int status;
            *(pidList+index) = pid;

            forker(nConversions-1,index+1,0,outputDesc,pipes,conversions,pidList);
            printf("going to wait\n");
            //waitpid(pid,&status,0);

        }
    }
}
void printerStatusChange(PRINTER* printer)
{
    printf("%s\n",imp_format_printer_status(printer, showBuf, 100));
}
PRINTER* findPrinterToPrint(PRINTER_SET eligiblePrinters,int sourceType)
{

    PRINTER_SET set = eligiblePrinters;
    int i = 0;
    while(i<32)
    {
        unsigned int eligible = set & 0x1;  //if the bit is set
        //printf("set:%x\n",set);
        if(eligible && !PRINTERS_LIST[i].busy)  //eligible and not busy
        {
            int dest = findFileIndex(PRINTERS_LIST[i].type);
            int * pathLength = (int*)malloc(sizeof(int));

            int found = findPath(sourceType,dest);
            *pathLength = found;
            if(found!=-1)
            {
                    debug("found depth");
                    PRINTERS_LIST[i].other_info = pathLength;
                    return &PRINTERS_LIST[i];
            }
        }
        set = set>>1;
        i++;
    }
    //went through all printers and did not find an eligible printer
    return NULL;


}
int retrieveFileType(char* fileName)
{
    char* type = findIndexOfDOT(fileName);
    if(findFileType(type))
    {
        //it is's a valid type
        return findFileIndex(type);
    }
    else
    {
            error("not a supported file type");
            return -1;
    }


}
char* findIndexOfDOT(char* fileName)
{
    int len = lengthOf(fileName);
    int i = 0;
    while(i<len-1)
    {

        if(*(fileName+i) == '.')
        {
            //printf("findIndexDOT %s\n",fileName+i+1);
            return fileName+i+1;
        }

        i++;
    }
    //if dot is the last character it's still invalid
    return NULL;

}
PRINTER_SET fillPrinterSet(int numArgs)
{
    PRINTER_SET eligiblePrinters = 0x0;
    for(int i = 2;i<numArgs;i++)
    {

        if(findPrinter(argVar[i]))
        {
            printf("%s found a printer\n",argVar[i]);
            eligiblePrinters |= (0x1<<(findPrinterID(argVar[i])));


            //found a declared printer
        }
        else
        {
            error("printer %s is not a declared printer type",argVar[i]);
            return 0; // don't fill anymore
        }
    }
    return eligiblePrinters;

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
                {
                        argForConvProg = argVar+4;
                        printf("arg for conv program%s\n",*argForConvProg);
                }
                int index1 = findFileIndex(argVar[1]);
                int index2 = findFileIndex(argVar[2]);

                CONVERSION c = {
                        index1,index2,
                        argVar[3],argForConvProg
                    };


                //debug("made conversion struct %d %d",index1,index2);

                conversionMatrix[index1][index2] = c; //matrix of conversion programs



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
            //debug("set of printers %x\nprinter name:%s",set,newPrinter.name);
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
int findPrinterID(char* name)
{
    int i;
    for(i= 0;i<imp_num_printers;i++)
    {
        if(strcmp(PRINTERS_LIST[i].name,name)==0 )
        {
            return PRINTERS_LIST[i].id;

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
    //char * buf = malloc(sizeof(char*)*100);
    //char *imp_format_printer_status(PRINTER *printer, char *buf, size_t size);
    for(int i = 0;i<imp_num_printers;i++)
    {
        printf("%s\n",imp_format_printer_status(&PRINTERS_LIST[i], showBuf, 100));
    }
    //free(buf);
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
    showBuf = (char*)malloc(100*sizeof(char));
    argCount = 5;
    jobsCreated = 0;
    jobQueue = NULL;

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
    //sleep(5);
    commandLineInterface();
    //testBFS();
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

            if(*buf==13)
                *temp++ = '\0';
            else
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

        //debug("currentChar: %d \n",*(line+size));
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
    //printf("size of arg: %d ",sizeOfArg);

    return argString(buffer,sizeOfArg);
}

