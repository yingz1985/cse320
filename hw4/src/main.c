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
#include <signal.h>


int continueExecution;
int CurrentNumberOfTypes;
int imp_num_printers;
PRINTER PRINTERS_LIST [MAX_PRINTERS];
char buffer[60];
char *bufferHead = buffer;
char *bufferEnd = buffer;
unsigned int sizeOfArg;
PRINTER_SET set;
char ** argVar;
int argCount;

CONVERSION path[32];
JOB* jobQueue;
int jobsCreated;
char* showBuf;


CONVERSION conversionMatrix[32][32];



void sigchildHandler(int sigchild)
{
    pid_t pid;
    int status;
    //sigset_t mask_all,prev_all;
    //sigfillset(&mask_all);
    while((pid= waitpid(-1,&status,WNOHANG|WUNTRACED))>0)
    {
        //sigprocmask(SIG_BLOCK,&mask_all,&prev_all);

        JOB* job = findJobCorrespondedWithPG(pid);
        printf("received sighchld signal from %d\n",pid);
        if(job)
        {
            if(WIFSTOPPED(status))
            {
                printf("received a signal to pause process %d\n",pid);
                //stopped by a signal, pause
                changeJobStatus(job,PAUSED);
            }
            else if(WIFCONTINUED(status))
            {
                printf("received a signal to continue process %d\n",pid);
                //stopped by a signal, pause
                changeJobStatus(job,RUNNING);
            }

            else
            {

                if(WIFEXITED(status))
                {
                    if(WEXITSTATUS(status)==EXIT_SUCCESS)
                    {
                        printf("process %d died normally with status %d\n",pid,status);
                        changeJobStatus(job,COMPLETED);
                    }
                    else
                    {
                        printf("child terminated abnormally,aborting\n");
                        changeJobStatus(job,ABORTED);
                    }

                }
                else
                {

                    printf("something went wrong, aborting");
                    changeJobStatus(job,ABORTED);


                }
                job->chosen_printer->busy = 0;  //no longer busy
                printerStatusChange(job->chosen_printer);
                executePendingJobsIfPossible();
            }
        }

       // sigprocmask(SIG_SETMASK,&prev_all,NULL);

    }
    printf("returning from sigchild handler\n");
}
JOB * findJobCorrespondedWithPG(pid_t pid)
{
    JOB* currentJob = jobQueue;
    while(currentJob!=NULL)
    {
        if(currentJob->pgid == pid)
        {
            return currentJob;
        }
        currentJob = currentJob->other_info;
    }
    return NULL;    //should never be null
}

void commandLineInterface()
{

    while(continueExecution)
    {
        char* line = readline("imp> ");
        if(line);
        {

            processCommands(line);
            free(line);
        }



    }


}
int parseLine(char*line)
{
    int length = lengthOf(line);
    int pos = 0;
    int count = 0;

    while((*line) != '\0' && (*line) !='\n')
    {


        int white = skipWhiteSpaces(line);
        line+=white;    // skip that many white spaces, could be zero if no white spaces
        count+=white;
        if(count>=length){
            //trailing empty spaces
            break;
        }
        char * arg = readArg(line);
        //debug("%s:arg",arg);
        if(pos==argCount)
        {
            //debug("realloc");
            argCount+=argCount;
            argVar = realloc(argVar,argCount*sizeof(char*));
        }
        argVar[pos++] = arg;
        line+=(bufferEnd-buffer-1); //move to next word
        count+=(bufferEnd-buffer-1);

    }
    argVar[pos] = "\0";

    return pos;
}
int findPath(int file1ID,int file2ID)
{

    if(file1ID==file2ID)
    {
        char** cat = calloc(2,sizeof(char*));
        cat[0] = "cat";
        CONVERSION* CON = (CONVERSION*)malloc(sizeof(CONVERSION));
        CONVERSION c = {
                        file1ID,file2ID,
                        *cat,cat
                    };
        *CON = c;
        path[0] = *CON; //path of 1 that contains cat
        return 0;   //zero conversions needed

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
        if(v==-1) break;    //queue is empty
        int i = 0;
        while(i<CurrentNumberOfTypes && v!=-1)
        {
            //
            if(conversionMatrix[v][i].path != '\0' && flag[i]==0)
                //if there's an edge, and vertex is unvisited
            {
                //debug("not null %d%d",v,i);
                //debug("%d%d %s",v,i,conversionMatrix[v][i].path);
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
    if(found!=-1)
    {
        printf("found path");
        int vertex = file2ID;
        int i = 0;
        while(prev[vertex]!=vertex && prev[vertex!=-1])
        {
            path[i] = conversionMatrix[prev[vertex]][vertex];   //last to first
            //printf("program path %s, args: %s\n",path[i].path,*path[i].argVar);
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


    }
    else
    {
        JOB* currentJob = jobQueue;//first job

        while(currentJob->other_info!=NULL)
        {
            //while job queue has next job in queue, keep going
            currentJob = ((JOB*)currentJob->other_info);
        }
        //currentJob.other_info = malloc(sizeof(JOB*));;
        currentJob->other_info = job;  //add job to end of queue

    }
    printf("%s\n",imp_format_job_status(job, showBuf, 100));
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
        if(!numArgs)
            return;

        if(strcmp(argVar[0],"help")==0 && numArgs==1)
        {

            printHelpMenu();
        }
        else if(strcmp(argVar[0],"type")==0)
        {

            if(numArgs<2)
            {
                    char* error = "requires type argument";
                    printererrorMessage(error);
            }
            else if (numArgs>2)
            {
                char* error = "extra arguments not needed was entered";
                printererrorMessage(error);
            }
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

            printf("printer set: %x",eligiblePrinters);
            int sourceType = retrieveFileType(argVar[1]);
            if(sourceType!=-1)
            {
                PRINTER* print = findPrinterToPrint(eligiblePrinters,sourceType);

                if(print==NULL)printf("not available printer");
                JOB* jobo = createJob(argVar[1],findIndexOfDOT(argVar[1]),eligiblePrinters,print);
                if(print)//if found a valid printer
                {

                        makeConversions(jobo);

                        printf("create job for %s\n",print->name);
                }
                else
                {
                    printf("did not find a printer to print, but queueing\n");
                }

                //did not find printer to print currently, but will queue the job
                //showJobs();

            }
            else
            {
                char* error = (char*)malloc(sizeof(char)*50);
                sprintf(error,"file extension of %s is not declared",argVar[1]);
                printererrorMessage(error);
                free(error);
            }

            //create job to be queued


        }
        else if(strcmp(argVar[0],"enable")==0)
        {
            if(numArgs==2)
            {
                if(findPrinter(argVar[1]))
                {
                    int index = findPrinterID(argVar[1]);
                    if(PRINTERS_LIST[index].enabled)   //if already enabled
                    {
                        char* error = (char*)malloc(sizeof(char)*50);
                        sprintf(error,"printer %s is already enabled",argVar[1]);
                        printererrorMessage(error);
                        free(error);
                    }
                    else
                    {
                        PRINTERS_LIST[index].enabled = 1;
                        printerStatusChange(&PRINTERS_LIST[index]);
                        executePendingJobsIfPossible();
                        //if can start job, start
                        //not coded yet
                    }

                }
                else
                {
                    char* error = (char*)malloc(sizeof(char)*50);
                    sprintf(error,"printer %s has not yet been declared",argVar[1]);
                    printererrorMessage(error);
                    free(error);
                }
            }
            else if(numArgs>2)
            {
                char* error = "too many arguments entered, command will be ignore";
                printererrorMessage(error);
            }
            else if(numArgs<2)
            {
                char* error = "Please supply a printer to enable";
                printererrorMessage(error);
            }
        }
        else if(strcmp(argVar[0],"disable")==0)
        {
            if(numArgs==2)
            {
                if(findPrinter(argVar[1]))
                {
                    int index = findPrinterID(argVar[1]);
                    if(!PRINTERS_LIST[index].enabled)   //if already disabled
                    {
                        char* error = (char*)malloc(sizeof(char)*50);
                        sprintf(error,"printer %s is already disabled",argVar[1]);
                        printererrorMessage(error);
                        free(error);
                    }
                    else
                    {
                        PRINTERS_LIST[index].enabled = 0;
                        printerStatusChange(&PRINTERS_LIST[index]);
                    }

                }
                else
                {
                    char* error = (char*)malloc(sizeof(char)*50);
                    sprintf(error,"printer %s has not yet been declared",argVar[1]);
                    printererrorMessage(error);
                    free(error);
                }
            }
            else if(numArgs>2)
            {
                char* error = "too many arguments entered, command will be ignore";
                printererrorMessage(error);
            }
            else if(numArgs<2)
            {
                char* error = "Please supply a printer to disable";
                printererrorMessage(error);
            }
        }
        else if(strcmp(argVar[0],"pause")==0)
        {
            if(numArgs==2)
            {
                int job_num = atoi(argVar[1]);  //convert string to job_num int
                JOB* jobo = findJobNum(job_num,RUNNING);


                if(jobo)
                {
                    //found
                    int processGroup = jobo->pgid;
                    printf("process group id: %d\n",processGroup);
                    //signal(SIGSTOP,pausedHandler);
                    killpg(processGroup,SIGSTOP);


                }

            }
            else if(numArgs==1)
            {
                char* error = "Please supply a job number to pause";
                printererrorMessage(error);
            }
            else
            {
                char* error = "Too many arguments entered, command will be ignored";
                printererrorMessage(error);
            }
        }
        else if(strcmp(argVar[0],"resume")==0)
        {
            if(numArgs==2)
            {
                int job_num = atoi(argVar[1]);  //convert string to job_num int
                JOB* jobo = findJobNum(job_num,PAUSED);


                if(jobo)
                {
                    //found
                    int processGroup = jobo->pgid;
                    printf("process group id: %d\n",processGroup);
                    //signal(SIGSTOP,pausedHandler);
                    killpg(processGroup,SIGCONT);   //continue processing


                }

            }
            else if(numArgs==1)
            {
                char* error = "Please supply a job number to pause";
                printererrorMessage(error);
            }
            else
            {
                char* error = "Too many arguments entered, command will be ignored";
                printererrorMessage(error);
            }
        }


        else if(strcmp(argVar[0],"quit")==0 &&  numArgs==1)
        {
                free(argVar[0]);
                continueExecution = 0;
        }

        if(jobQueue)
            removeFinishedJobs();


    //free(argVar[0]);

}
void removeFinishedJobs()
{
    JOB* currentJob = jobQueue;

    while(currentJob!=NULL)
    {
        struct timeval currentTime;
        gettimeofday(&currentTime,NULL);

        if(jobQueue->status==COMPLETED || jobQueue->status==ABORTED)
        {
            if(currentTime.tv_sec-jobQueue->change_time.tv_sec>60)
                jobQueue = jobQueue->other_info;
            //been finished more than a minute ago
            //if head should be removed, remove head
        }

        JOB* nextJob = (JOB*)currentJob->other_info;
        if(nextJob!=NULL &&(nextJob->status==COMPLETED || nextJob->status ==ABORTED))
        {
            if(currentTime.tv_sec-jobQueue->change_time.tv_sec>60)
                currentJob->other_info = nextJob->other_info;
            //skip over next jobo
        }

        currentJob = (JOB*)currentJob->other_info;

    }
}
JOB* findJobNum(int job_num,JOB_STATUS status)
{
    JOB* currentJob = jobQueue;
    //char * buf = malloc(sizeof(char)*100);
    while(currentJob!=NULL)//if job not yet performed, check
    {
        if(currentJob->jobid==job_num)
        {
            if(currentJob->status==status)
            {
                return currentJob;//return process group id
            }
            else
            {
                char* error = (char*)malloc(sizeof(char)*50);
                sprintf(error,"job currently does not support operation selected");
                printererrorMessage(error);
                free(error);
                return NULL;
            }

        }
        currentJob = (JOB*)currentJob->other_info;//go to next jobo

        //search if it's possible to start job
    }

    return NULL;
}
void executePendingJobsIfPossible()
{
    JOB* currentJob = jobQueue;
    //char * buf = malloc(sizeof(char)*100);
    while(currentJob!=NULL)//if job not yet performed, check
    {
        if(currentJob->status==QUEUED)
        {
            int sourceType = findFileIndex(currentJob->file_type);

            PRINTER* print = findPrinterToPrint(currentJob->eligible_printers,sourceType);

            if(print==NULL)
                printf("not available printer\n");
            else
            {
                printf("found printer to print %s\n",print->name);
                currentJob->chosen_printer = print;
                makeConversions(currentJob);
            }
        }

        currentJob = (JOB*)currentJob->other_info;//go to next job if it's not processing

        //search if it's possible to start job
    }
}
void changeJobStatus(JOB* job,JOB_STATUS status)
{
    struct timeval currentTime;

    gettimeofday(&currentTime,NULL);

    job->status = status;
    job->change_time = currentTime;
    //printf("%s\n",imp_format_job_status(job, showBuf, 100));
    char* localBuf =imp_format_job_status(job, showBuf, 100);
    size_t nbytes = lengthOf(localBuf);
    write(1,localBuf,nbytes);
    write(1,"\n",1);

}
void makeConversions(JOB * job)
{
    //assuming that a valid printer was found and path was also found accordingly
    //assuming size is not zero
    //this is the method that makes the processes **

    int conversionsNeeded = *(int*)job->chosen_printer->other_info;
    printf("conversions needed %d",conversionsNeeded);

    if(conversionsNeeded == 0)
    {
        printf("no conversions needed\n");
        conversionsNeeded++;
    }
    CONVERSION* localCopy = (CONVERSION*)malloc(conversionsNeeded*sizeof(CONVERSION));
    int c = conversionsNeeded;

    int conversionPipes[c][2];//[conversionsNeeded];    //initially had size 0 array, not allowed

    memcpy(localCopy,path,conversionsNeeded*sizeof(CONVERSION));

    //make a local copy of the conversion pipes

    pid_t pid;

    int * processList = (int*)malloc(conversionsNeeded*sizeof(int));


    changeJobStatus(job,RUNNING);
    job->chosen_printer->busy = 1;  //currenty busy
    printerStatusChange(job->chosen_printer);




    if((pid=fork())==0)
    {
        setpgid(getpid(),getpid()); // get process group id to itself
        //master process

        printf("master process is running%d\n",getpid());
        int infile = open(job->file_name,O_RDONLY);
        int printerDesc = imp_connect_to_printer(job->chosen_printer, 0x0);
        //fprintf(stderr,"connect to printer %d infile %d\n",printerDesc,infile);
        //fflush(stderr);

        //printf("created master process\n");
        //printf("conversions %d\n",conversionsNeeded);

        forker(conversionsNeeded,0,infile,printerDesc,conversionPipes,localCopy,processList);


        for(int i = 0;i<conversionsNeeded-1;i++)
        {

           close(conversionPipes[i][0]);
           close(conversionPipes[i][1]);


            //opening up pipes before forking
        }
        //printf("forked\n");
        int status = 0;
        for(int i = 0;i<conversionsNeeded;i++)
        {

            //fprintf(stderr,"waiting for %d\n",processList[i]);
            int p = 0;
            while((p= waitpid(processList[i],&status,0))>0)
            {
                //fprintf(stderr,"return of wait pid %d\n",p);
                fflush(stderr);


                if(WIFEXITED(status))
                {
                    if(WEXITSTATUS(status)==EXIT_SUCCESS)
                    {
                        fprintf(stderr,"child %d exited with status %d\n",processList[i],status);
                        fflush(stderr);
                    }
                    else
                    {
                        printf("something went wrong,master process exiting w/ failure\n");
                        _exit(EXIT_FAILURE);
                    }
                    //normal exit
                }
                else
                {

                    fprintf(stderr,"child %d terminated abnormally with status %d\n",processList[i],status);
                    fflush(stderr);
                    _exit(EXIT_FAILURE);
                }
            }
            /*if(p==-1)
            {
                    printf("error %d\n",p);
                    _exit(EXIT_FAILURE);
            }*/
        }
        printf("exiting master process, conversions made successfully\n");


        _exit(EXIT_SUCCESS);

    }
    else
    {
        //main- do nothing
        //printf("continueMain\n");
        signal(SIGCHLD,sigchildHandler);//installed a signal
        job->pgid = pid;//set process group id for jobo
        //setpgid(pid,pid);
        //waitpid(pid,&status,0);
    }



}

void forker(int nConversions,int index,int fileDesc,int outputDesc,int pipes[][2],CONVERSION* conversions,int* pidList)
{
    if(index==0)
    {
        for(int i = 0;i<nConversions-1;i++)
        {

                if(pipe(pipes[i])==-1)
                {
                    printf("pipe failed\n");
                    _exit(EXIT_FAILURE);
                };
                close(pipes[i][0]); //parent doesn't use pipes so close them
                close(pipes[i][1]);


            //connect pipes before forking
        }
    }
    pid_t pid;
    if(nConversions>0)
    {

      if(pipe(pipes[index])==-1)
      {
        printf("pipe failed\n");

      };



        //printf("created pipe %d\n",index);

        if((pid = fork()) == 0)
        {

            //fprintf(stderr,"child %d's process group id is %d\n",getpid(),getpgrp());


            //cannot have pipe in child processes
            //it becomes unknown

            //child inherits the process group of the parent


            if(nConversions==1) //output redirection
            {
                //output to printer descriptor
                //fprintf(stderr,"flushing to printer %d\n",outputDesc);
                //fflush(stderr);
                //just print
                int d = dup2(outputDesc,1) ;
                if(d==-1)
                {
                    //fprintf(stderr,"bad printer file descriptor %d\n",outputDesc);
                    //fflush(stderr);
                    _exit(EXIT_FAILURE);
                }



            }

            else
            {
                //fprintf(stderr,"piping output %d\n",index);


                int d = dup2(pipes[index][1],1);
                close(pipes[index][0]);    //close read-side of current pipe

                 if(d==-1)
                {
                    //fprintf(stderr,"output bad file descriptor %d\n",pipes[index][1]);

                    _exit(EXIT_FAILURE);
                }
                    //if index==0, it has no prev pipe, reads from file


                //output to write end of pipe wait to be read


            }


            if(!index)//input redirection
            {//index == 0; first child process has to get input from fileDesc

                //fprintf(stderr,"reading from input file desciptor %d\n",fileDesc);

                int d = dup2(fileDesc,0);
                if(d==-1)
                {
                    /*fprintf(stderr,"input bad file descriptor %d\n",fileDesc);
                    fflush(stderr);*/
                    _exit(EXIT_FAILURE);
                }
                //close(fileDesc);  //close the reading side since we'll read from file


            }

            else
            {
                //fprintf(stderr,"reading from previous %d \n",index);
                //fflush(stderr);
                close(pipes[index-1][1]);//close previous output to enable reading
                int d = dup2(pipes[index-1][0],0);  //take input from previous output


                if(d==-1)
                {
                    /*fprintf(stderr,"not first input bad file descriptor index:%d %d\n",index,pipes[index][1]);
                    fflush(stderr);*/
                    _exit(EXIT_FAILURE);
                }

                //read from output of previous pipe
            }
            //printf("numConv: %d path:%s arg:%s\n",nConversions-1,conversions[nConversions-1].path,*(conversions[nConversions-1].argVar));

            for(int i = 0;i<(nConversions+index-1);i++)
            {   //close all unused pipes
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            int execute = execvp(conversions[nConversions-1].path,conversions[nConversions-1].argVar);
            //if execvp returned it must've failed
            //printf("execution failed");
            if(execute==-1)
            {
                _exit(EXIT_FAILURE);
            }
            _exit(EXIT_FAILURE);


        }
        else if(pid<0)
        {
            _exit(EXIT_FAILURE);//fork creation failed
        }
        else
        {

            //printf("process %d at index %d\n",pid,index);

            *(pidList+index) = pid;

            forker(nConversions-1,index+1,fileDesc,outputDesc,pipes,conversions,pidList);


            //printf("going to wait\n");
            //waitpid(pid,&status,0);

        }
    }
}
void printerStatusChange(PRINTER* printer)
{
    char * buf = imp_format_printer_status(printer, showBuf, 100);
    size_t length = lengthOf(buf);
    write(1,buf,length);
    write(1,"\n",1);
}

void printererrorMessage(char* message)
{
    printf("%s\n",imp_format_error_message(message, showBuf, 100));
}
PRINTER* findPrinterToPrint(PRINTER_SET eligiblePrinters,int sourceType)
{

    PRINTER_SET set = eligiblePrinters;
    int i = 0;
    while(i<32&& i<imp_num_printers)
    {
        unsigned int eligible = set & 0x1;  //if the bit is set
        //printf("set:%x\n",set);
        printf("printer name: %s busy: %d enabled:%d\n",PRINTERS_LIST[i].name,PRINTERS_LIST[i].busy,PRINTERS_LIST[i].enabled);
        if(eligible && !PRINTERS_LIST[i].busy && PRINTERS_LIST[i].enabled)  //eligible and not busy
        {
            int dest = findFileIndex(PRINTERS_LIST[i].type);
            int * pathLength = (int*)malloc(sizeof(int));

            int found = findPath(sourceType,dest);
            *pathLength = found;
            if(found!=-1)
            {
                    debug("found depth");   //zero is fine too
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

    if(type!=NULL && findFileType(type))
    {
        //it is's a valid type
        return findFileIndex(type);
    }
    else
    {
            char* error = "not a supported file type";
            printererrorMessage(error);

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
            char * error = (char*)malloc(sizeof(char)*50);
            sprintf(error,"printer %s is not a declared printer type",argVar[i]);
            free(error);
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

                char** argForConvProg = calloc((numArgs-3),sizeof(char*));
                memcpy(argForConvProg,argVar+3,(numArgs-3)*sizeof(char*));
                //argForConvProg = argVar+3;

                int index1 = findFileIndex(argVar[1]);
                int index2 = findFileIndex(argVar[2]);
                CONVERSION * conv = (CONVERSION*)malloc(sizeof(CONVERSION));

                CONVERSION c = {
                        index1,index2,
                        argVar[3],argForConvProg
                    };
                *conv = c;


                //debug("made conversion struct %d %d",index1,index2);

                conversionMatrix[index1][index2] = *conv; //matrix of conversion programs
                printf("argVar: %s ",*(argVar+3));
                printf("arg for conversion program %s\n",*c.argVar);



            }
            else
            {
                //debug("%s:%d %s:%d",argVar[1],find1,argVar[2],find2);
                char* error = "one or more file type(s) are not supported";
                printererrorMessage(error);
            }

    }
    else
    {
        //debug("%s: %s",argVar[1],argVar[2]);
        char* error = "not enough arguments were supplied";
        printererrorMessage(error);
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
        char* error = "extra arguments not needed was inputted ";
        printererrorMessage(error);
    }
    else
    {
        char* error = "must input both printer name and file type ";
        printererrorMessage(error);
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
            PRINTER* printer = (PRINTER*)malloc(sizeof(PRINTER));
            PRINTER newPrinter = {imp_num_printers,printerName,fileName,0,0,NULL};
            *printer = newPrinter;
            PRINTERS_LIST[imp_num_printers++] = *printer;
            set|= (0x1<<newPrinter.id);
            printerStatusChange(&newPrinter);
            //debug("set of printers %x\nprinter name:%s",set,newPrinter.name);
        }
        else
        {
            char* error = (char*)malloc(sizeof(char)*50);
            sprintf(error,"file type %s is not an accepted type",fileName);
            printererrorMessage(error);
            free(error);
        }
    }
    else
    {
        char* error = (char*)malloc(sizeof(char)*50);
        sprintf(error,"printer %s has already been declared",printerName);
        printererrorMessage(error);
        free(error);

    }



}
int findPrinter(char* name)
{
    int i;
    for(i= 0;i<imp_num_printers;i++)
    {

        if(strcmp(PRINTERS_LIST[i].name,name)==0 )
        {
            //printf("printer name : %s i = %d\n",PRINTERS_LIST[i].name,i);
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
        TYPE * type = (TYPE*)malloc(sizeof(TYPE));
        TYPE newType = {CurrentNumberOfTypes,name};
        *type = newType;
        TYPES_LIST[CurrentNumberOfTypes++] = *type;
        //debug("type name %s1",name);
    }
    else
    {
        char* error = (char*)malloc(sizeof(char)*50);
        sprintf(error,"type %s has already been declared",name);
        printererrorMessage(error);
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
    continueExecution = 1;
    sizeOfArg = 0;
    argVar = calloc(argCount,sizeof(char*));//*sizeof(char(*)));
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
                {
                        char* error = "failed to open file";
                        printererrorMessage(error);
                }
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
        char* line = (char*)calloc(80,sizeof(char));
        while(fgets(line,80,infile)!=NULL)
        {

            processCommands(line);
        }
        //printConversionprograms();

    }

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
        char* error = "memory not allocated for string";
        printererrorMessage(error);
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
    if(i==32 || i=='\t'||i==' ')   //don't skip on new line
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
    while(!isWhiteSpace(*(line+size)) && *(line+size)!='\0')
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

