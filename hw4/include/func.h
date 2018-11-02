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