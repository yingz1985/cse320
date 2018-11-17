char* argString(char*buf,int size);
void clearBuffer();
int isWhiteSpace(char i);
int skipWhiteSpaces(char*line);
void readUntilWhitespace(char*line);
char* readArg(char*line);
void printHelpMenu();
int lengthOf(char* string);
void showTypes();
void addType();
int findFileType(char*name);
void addPrinter();
int findPrinter(char* name);
void processCommandPrinter(int args);
void processCommandConversion(int args);
void processCommands(char*line);
int findFileIndex(char*name);
void printConversionprograms();
void showPrinters();
void showJobs();
int findPath(int file1ID,int file2ID);
void initializeArray(int * array,int size,int val);
PRINTER_SET fillPrinterSet(int numArgs);
int findPrinterID(char* name);
char* findIndexOfDOT(char* fileName);
int retrieveFileType(char* fileName);
PRINTER* findPrinterToPrint(PRINTER_SET eligiblePrinters,int sourceType);
JOB* createJob(char* file_name,char*file_type,PRINTER_SET set,PRINTER *chosen);
char* findIndexOfDOT(char* fileName);
void makeConversions(JOB * job);
void printererrorMessage(char* message);
void printerStatusChange(PRINTER* printer);
void commandLineInterface();
void executePendingJobsIfPossible();
JOB* findJobNum(int job_num,JOB_STATUS status);
void changeJobStatus(JOB* job,JOB_STATUS status);
void removeFinishedJobs();
JOB * findJobCorrespondedWithPG(pid_t pid);
void freeStructures();
int isNum(char* c);


typedef struct file_type{
    int id;
    char* name;
}TYPE;


typedef struct conversion{
    int from;//index of file1 in array
    int to;//index of file2 in array
    char* path;//path of conversion program
    char** argVar;//arguments, if any

}CONVERSION;


#define MAX_TYPES 32
extern int CurrentNumberOfTypes;
TYPE TYPES_LIST [MAX_TYPES];
void forker(int nConversions,int index,int fileDesc,int desc,int pipes[][2],CONVERSION* conversions,int* pidList);
//void forker(int nConversions,int index,JOB*job,int pipes[][2],CONVERSION* conversions,int* pidList);