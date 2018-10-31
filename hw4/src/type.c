

#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "type.h"

char buffer[30];
char *bufferHead = buffer;
char *bufferEnd = buffer;
int sizeOfArg = 0;

char * argString(char*buf,int size)
{
    char* string,*temp;
    if((string = (char*)malloc(size))==NULL)
    {
        error("memory not allocated for string")
        return NULL;
    }
    else
    {
        temp = string;
        while(size-->0)
        {
            *temp++ = *buf++;
        }
        return string;
    }


}
void clearBuffer()
{
    bufferHead=bufferEnd=buffer;
}

int isWhiteSpace(char i)
{
    if(i==' ' || i== '\n' || i=='\t')
        return 1;
    return 0;
}
int skipWhiteSpaces(char*line)
{
    int space =0;
    while(isWhiteSpace(*line++))
    {
        space ++;
    }
    return space;
}

void readUntilWhitespace(char* line)
{
    clearBuffer();
    int size = 0;
    line+=skipWhiteSpaces(line);
    while(!isWhiteSpace(*(line++)))
    {
        size++;
        *bufferEnd++ = *(line-1);
    }
    if(bufferEnd != buffer)
        *bufferEnd++ = '\n';

}
char* readArg(char* line)   //returns string of only the argument
{
    readUntilWhitespace(line);
    sizeOfArg = bufferEnd-bufferHead;
    return argString(buffer,sizeOfArg);
}

