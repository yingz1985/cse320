#include <stdlib.h>

#include "debug.h"
#include "hw1.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the content of three frames of audio data and
 * two annotation fields have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

//returns length of string passed in
unsigned int stringLength(char *string)
{

    int i = 1;
    int count = 0;
    while(i)
    {
       if( *(string+count)!='\0' )
       {
            count++;
       }
       else
       {
            i = 0;
       }
    }
    return count;
}

//returns 1 if key is valid, 0 else
int validateKey(char *string)
{
    int i = 1;
    int count = 0;
    while(*(string+count)!='\0' && i==1)
    {
       if(   ( *(string+count)>='A'&&*(string+count)<='F' )
        ||   ( *(string+count)>='a'&&*(string+count)<='f')
        ||   (*(string+count)>='0' &&*(string+count)<='9'))
       {
            //check key
            count++; //current check passed

       }
       else
       {
            i = 0;
       }
    }
    if(count>8) return 0;

    return i;
}

//assumes that key has already been verified
unsigned long convertKey(char* string)
{

    unsigned long total= 0;
    int size = stringLength(string);
    int count = 1;

    while(*(string+count-1)!='\0' )
    {
        int factor = 4*(size -count);
        unsigned long value;
       if(*(string+count-1)>='0' &&*(string+count-1)<='9')
       {
            value = *(string+count-1)-'0';

            //if(factor==0)
              //  total = total + value;
            //else
                total = total + (value<<factor);

       }
       else if( *(string+count-1)>='A' && *(string+count-1)<='F' )
       {
            value = *(string+count-1)-'A'+10;
            //if(factor==0)
              //  total = total + value;
            //else
                total = total + (value<<factor);

       }
       else if( *(string+count-1)>='a' && *(string+count-1)<='f')
       {

            value = *(string+count-1)-'a' +10;
            //if(factor==0)
              //  total = total + value;
            //else
                total = total + (value<<factor);
       }
       count++;
    }


    return total;

}

//a self-implemented power function of 10
int powerOfTen(int power)
{
    if(power == 0)
        return 1;   //base case
    int ten = 10;
    int i = 1;
    while(i<power)
    {
        ten= ten*10;
        i++;
    }

    return ten;
}

//returns 1 if factor within range and does not have random inputs
unsigned long validateFactor(char *string)
{
    int length = stringLength(string);
    int total = 0;
    int i = 1;
    int count = 0;
    while(*(string+count)!='\0'&&i==1)
    {
       if( *(string+count)>='0' &&  *(string+count)<='9')//verify that it's a digit
       {
            int num = (*(string+count))-'0';
            count++;
            total += powerOfTen(length-count) * num;
            //printf("powTen:%d\n%d\nnum:%d",powerOfTen(length-count),total,num);

       }
       else
       {
            i = 0;
       }
    }
    //if value is out of range or stopped short because there's an invalid input some where
    //return 0;
    if(total>1024||total<1||i==0) return 0;
    return total;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 1 if validation succeeds and 0 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variables "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 1 if validation succeeds and 0 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv)
{
    //int hFlag=0;
    global_options = 0;
    int uFlag=0;
    int dFlag=0;
    int cFlag=0;
    //int pFlag=0;
    int kflag=0;
    int keyFound = 0;
    int fFlag=0;
    int factorFound = 0;
    int i;  //counter
    if(argc>1)
    {
        for(i=1;i<argc;i++)
        {

            int argSize = stringLength(*(argv+i));
            char dash = **(argv+i);
            char flag = *(*(argv+i)+1);
            //check for validArguments
            if(i==1) //the following are the only accepted conditions for the first command
            {
                if( dash=='-' && flag=='h' && argSize==2)
                {
                   // hFlag =1; //detected hFlag
                    unsigned long h = 1UL<<63;
                    global_options = h | global_options;


                  //  printf("detected 'h' flag and set global_options %lx\n",global_options);
                    return 1;
                }
                else if (dash =='-' && flag =='c' && argSize ==2)
                {
                    unsigned long h = 1UL<<60;
                    global_options = h | global_options;
                    //printf("detected c flag and set global_options %lx\n",global_options);
                    cFlag = 1; //found cFlag
                }
                else if(dash=='-' && flag =='d' && argSize == 2)
                {
                    unsigned long h = 1UL<<61;
                    global_options = h | global_options;
                   // printf("detected d flag and set global_options %lx\n",global_options);
                    dFlag = i;
                }
                else if(dash=='-' && flag =='u' && argSize ==2)
                {
                    unsigned long h = 1UL<<62;
                    global_options = h | global_options;
                   // printf("detected u flag and set global_options %lx\n",global_options);
                    uFlag = i;
                }
                else
                {
                   // printf("bad input, optional arguments cannot come first\n");
                    global_options = 0;
                    return 0;   //detect bad inputs before going any further
                }


            }
            else
            {
                if(argc>5)
                {
                    global_options = 0;
                   // printf("too many arguments and set global_options %lx\n",global_options);
                    return 0;
                }
                else if(dash=='-' && flag =='f' && argSize ==2 && (uFlag==1 || dFlag ==1))
                {

                    //only perform factor if either -u or -d command is found
                    fFlag = i;

                   // printf("factor command found after speedup or slowdown command\n");
                }
                else if(dash =='-' && flag =='k' && argSize ==2 && cFlag ==1)
                {
                    kflag = i; //note the location of k because key should follow k
                    //printf("k flag found after c flag\n ");
                }
                else if(dash =='-' && flag =='p' && argSize ==2)
                {
                    //pFlag = 1;
                    unsigned long h = 1UL<<59;
                    global_options = h | global_options;
                  //  printf("optional p flag found and set global_options %lx\n",global_options);
                }
                else if(kflag+1==i && kflag!=0)
                {
                    //decode the key
                    //printf("key is found\n");
                    keyFound = 1;
                    int val = validateKey((*(argv+i)));
                    kflag = 1;
                    if (val==0)
                    {
                        global_options = 0;
                       // printf("finally %lu",global_options);
                        return 0; //invalid key
                    }
                    else
                    {

                            unsigned long key = convertKey(*(argv+i));
                            global_options = key | global_options;
                            //printf("verified key and set global_options %lx\n",global_options);
                    }

                    //else proceed to check next
                }
                else if(fFlag+1==i && fFlag!=0)
                {
                    factorFound = 1;
                    fFlag = 1;//reset fFlag after valid flag is found
                    unsigned long factor = validateFactor((*(argv+i)));
                    if(factor!=0)
                    {
                        factor = factor -1;
                        unsigned long h = factor<<48;
                        global_options = h | global_options;
                    }
                    else
                    {
                        global_options = 0;
                      //  printf("finally %lu",global_options);
                        return 0;
                    }


                    //printf("factor is found and set global_options %lx\n",global_options);
                }
                else
                {
                    //printf("Not a valid command option\n");
                    global_options = 0;
                    return 0;//if input doesnt match these conditions, it is invalid
                }

            }
        }

        if(kflag==keyFound && fFlag==factorFound && cFlag == kflag )
        {
            return 1;
        }


    }
            //printf("bad");
                global_options = 0;
                return 0;

    }





int appendString(char* output,char*input,int offset)
{
    int i = 0;
    while(*(input+i)!='\0')//while not at the end of the input string, keep appending
    {

        *(output+offset) = *(input+i);
        offset++;
        i++;
    }

    return offset;

}







/**
 * @brief  Recodes a Sun audio (.au) format audio stream, reading the stream
 * from standard input and writing the recoded stream to standard output.
 * @details  This function reads a sequence of bytes from the standard
 * input and interprets it as digital audio according to the Sun audio
 * (.au) format.  A selected transformation (determined by the global variable
 * "global_options") is applied to the audio stream and the transformed stream
 * is written to the standard output, again according to Sun audio format.
 *
 * @param  argv  Command-line arguments, for constructing modified annotation.
 * @return 1 if the recoding completed successfully, 0 otherwise.
 */
int recode(char **argv) {
    static AUDIO_HEADER hp;
    //printf("begin method\n");
    //static AUDIO_HEADER *hp = &p;//declare an empty audio header and pre-initialized to 0
    int readHeader = read_header(&hp);
    //printf("read header\n");
    if(readHeader==0) return 0;
    unsigned int bytes = 0;

    int offset = hp.data_offset-sizeof(hp); //size of annotation
    //printf("size of annotation is%d\n",offset);
    if(offset>ANNOTATION_MAX) return 0;

    int readAnnotation = read_annotation(input_annotation,offset);
    if(readAnnotation==0) return 0; //not null terminated
    //printf("annotation read successfully\n");


    if(global_options & (0x1L << 59)) // if -p is set, output annotation = input annotation
    {
        offset = hp.data_offset-sizeof(hp); //offset = size of annotation
        //bytes = offset;
        bytes = appendString(output_annotation,input_annotation,bytes);

            while(hp.data_offset%8!=0)
        {
            if(bytes+1>=ANNOTATION_MAX) return 0;
            *(output_annotation+(bytes++)) = '\0';
            hp.data_offset = (bytes+24);
        }
        write_header(&hp);    //will write in the end
        write_annotation(output_annotation,bytes);

    }
    else
    {

        bytes = 0;
        int i = 0;


        while(*(argv+i)!=NULL&& bytes<ANNOTATION_MAX)
        {


            if(bytes>ANNOTATION_MAX)  return 0; //error if annotation is too long
            bytes = appendString(output_annotation,*(argv+i),bytes);
            if(*(argv+i+1)!=NULL)
                *(output_annotation+(bytes++))  = ' ';

            i++;

        }

        if(bytes+1>=ANNOTATION_MAX) return 0;
        if(offset==0)
            *(output_annotation+(bytes++)) = '\0'; //null terminated
        else
            *(output_annotation+(bytes++)) = '\n';


        bytes = appendString(output_annotation,input_annotation,bytes);

        hp.data_offset = bytes;
        while(hp.data_offset%8!=0 || hp.data_offset<32)
        {
            if(bytes+1>=ANNOTATION_MAX) return 0;
            *(output_annotation+(bytes++)) = '\0';
            hp.data_offset = (bytes+24);
        }


        write_header(&hp);
        write_annotation(output_annotation,bytes);

    }

    if(global_options & (0x1L << 62))//if speed up oeration is set
    {
        //read every n'th frame
        unsigned int size = hp.channels*(hp.encoding-1);
        //printf("frame size: %u",size);

        if(hp.data_size % size != 0 && hp.data_size!=0xffffffff ) return 0;

        unsigned int i = ((global_options<<6)>>54)+1;
        //(0x3ffL & global_options)  +1 ; //factor bits retrieved
        //printf("factor: %u",i);
        unsigned int k = 1;
         //140780 printf("data size: %u",hp.data_size);

        while(k<(hp.data_size/size))
        {

            int u = read_frame((int*) input_frame,hp.channels,hp.encoding-1);
            if(u==0) return 0;

            if( k % i == 0)
            {
            //int* address = input_frame;
              //int m  =
               write_frame((int*)input_frame,hp.channels,hp.encoding-1);
             // if(m!=0) write_frame((int*)output_frame,hp.channels,hp.encoding-1);
             // else return 0;

            }

            k++;


        }
    }
    else
    {

    }

    if(global_options & (0x1L << 61))//if slow down operation is set
    {

    }
    else
    {

    }



    return 0;
}


/**
 * @brief Read the header of a Sun audio file and check it for validity.
 * @details  This function reads 24 bytes of data from the standard input and
 * interprets it as the header of a Sun audio file.  The data is decoded into
 * six unsigned int values, assuming big-endian byte order.   The decoded values
 * are stored into the AUDIO_HEADER structure pointed at by hp.
 * The header is then checked for validity, which means:  no error occurred
 * while reading the header data, the magic number is valid, the data offset
 * is a multiple of 8, the value of encoding field is one of {2, 3, 4, 5},
 * and the value of the channels field is one of {1, 2}.
 *
 * @param hp  A pointer to the AUDIO_HEADER structure that is to receive
 * the data.
 * @return  1 if a valid header was read, otherwise 0.
 */
int read_header(AUDIO_HEADER *hp)
{
    int element;
    int byte;
    unsigned int value = 0;
    /*for(element = 0;element<24;element++)
    {

        printf("%c",getchar());
    }*/
    for(element = 0; element<6;element++)
    {
        value = 0;
        for(byte = 1;byte<=4;byte++)
        {

            unsigned char part = getchar();
            value = value + part;
            if(byte!=4)
                value = value <<8 ;

        }

       /* if(element == 0)
        {
            hp->magic_number = value;
            //printf("%x\n",value);
        }
        else if(element == 1)
        {
            hp->data_offset =value;
            //printf("%x\n",value);
        }
        else if(element == 2)
        {
            hp->data_size =value;
           // printf("%x\n",value);
        }
        else if(element == 3)
        {
            hp->encoding =value;
            //printf("%x\n",value);

        }
        else if(element == 4)
        {
            hp->sample_rate =value;
            //printf("%x\n",value);
        }
        else if(element == 5)
        {
            hp->channels =value;
            //printf("%x\n",value);
        }*/

        *(&(hp->magic_number)+element ) = value;
        /*
        *   base_address starts at magic_number
        *   adding element is equivalent to adding size(int) to the base address
        *
        *
        */

        //printf("%x\n",*(&(hp->magic_number)+element ));

    }


    if(hp->magic_number != 0x2e736e64)
    {

        return 0;
    }
    else if(hp->data_offset %8 != 0)
    {
        return 0;
    }
    else if(hp->encoding>5 || hp-> encoding<2)
    {
        return 0;
    }
    else if(hp->channels>2||hp->channels<1)
    {
        return 0;
    }
    else
    {
        //printf("successful");
        return 1;

    }
}


/**
 * @brief  Write the header of a Sun audio file to the standard output.
 * @details  This function takes the pointer to the AUDIO_HEADER structure passed
 * as an argument, encodes this header into 24 bytes of data according to the Sun
 * audio file format specifications, and writes this data to the standard output.
 *
 * @param  hp  A pointer to the AUDIO_HEADER structure that is to be output.
 * @return  1 if the function is successful at writing the data; otherwise 0.
 */
int write_header(AUDIO_HEADER *hp)
{
     unsigned int *pointer = &(hp->magic_number); //grab the address of the first element
     int i, k;
     for(i = 0; i<6; i++)
     {
        int value = *pointer; //grab the first integer value
        for(k=1;k<=4;k++)
        {
           unsigned int temp = value>>8*(4-k);
           printf("%c",temp);
        }
        pointer = pointer+1;

     }
/*
     pointer = &(hp->data_offset);

        value = *pointer;
        for(k=1;k<=4;k++)
        {
           unsigned int temp = value>>8*(4-k);
           printf("%c",temp);
        }


     pointer = &(hp->data_size);

        value = *pointer;
        for(k=1;k<=4;k++)
        {
           unsigned int temp = value>>8*(4-k);
           printf("%c",temp);
        }

    pointer = &(hp->encoding);
        value = *pointer;
        for(k=1;k<=4;k++)
        {
           unsigned int temp = value>>8*(4-k);
           printf("%c",temp);
        }
    pointer = &(hp->sample_rate);

        value = *pointer;
        for(k=1;k<=4;k++)
        {
           unsigned int temp = value>>8*(4-k);
           printf("%c",temp);
        }
    pointer = &(hp->channels);

        value = *pointer;
        for(k=1;k<=4;k++)
        {
           unsigned int temp = value>>8*(4-k);
           printf("%c",temp);
        }
*/

    return 1;
}

/*
    helper function toString();

*/
void printString(char *string,unsigned int size)
{

    int count = 0;
    /*(string+count)!='\0'
        originally included
        but null characters are part of the audio file
    */
    while(size>0)   //print to output even if there're nulls
    {

       printf("%c",*(string+count));
       count++;
       size--;
    }

}


/**
 * @brief  Read annotation data for a Sun audio file from the standard input,
 * storing the contents in a specified buffer.
 * @details  This function takes a pointer 'ap' to a buffer capable of holding at
 * least 'size' characters, and it reads 'size' characters from the standard input,
 * storing the characters read in the specified buffer.  It is checked that the
 * data read is terminated by at least one null ('\0') byte.
 *
 * @param  ap  A pointer to the buffer that is to receive the annotation data.
 * @param  size  The number of bytes of data to be read.
 * @return  1 if 'size' bytes of valid annotation data were successfully read;
 * otherwise 0.
 */
int read_annotation(char *ap, unsigned int size)
{
    int i;
   /*
    for(i = 0;i<24;i++)
    {
         getchar();

        //disregard the first 24 bytes
    }
    */
    for(i = 0;i<size;i++)
    {

        char character = getchar();
        if(character==EOF) return 0;
        *(ap+i) = character;
        //printString(ap,size);
        if(i==size-1  &&     character!='\0'  )
        {
            printf("bad");
            return 0;
        }
    }

    //printString(ap,size);
    return 1;

}



/**
 * @brief  Write annotation data for a Sun audio file to the standard output.
 * @details  This function takes a pointer 'ap' to a buffer containing 'size'
 * characters, and it writes 'size' characters from that buffer to the standard
 * output.
 *
 * @param  ap  A pointer to the buffer containing the annotation data to be
 * written.
 * @param  size  The number of bytes of data to be written.
 * @return  1 if 'size' bytes of data were successfully written; otherwise 0.
 */
int write_annotation(char *ap, unsigned int size)
{
    printString(ap,size);
    return 1;
}




/**
 * @brief Read, from the standard input, a single frame of audio data having
 * a specified number of channels and bytes per sample.
 * @details  This function takes a pointer 'fp' to a buffer having sufficient
 * space to hold 'channels' values of type 'int', it reads
 * 'channels * bytes_per_sample' data bytes from the standard input,
 * interpreting each successive set of 'bytes_per_sample' data bytes as
 * the big-endian representation of a signed integer sample value, and it
 * stores the decoded sample values into the specified buffer.
 *
 * @param  fp  A pointer to the buffer that is to receive the decoded sample
 * values.
 * @param  channels  The number of channels.
 * @param  bytes_per_sample  The number of bytes per sample.
 * @return  1 if a complete frame was read without error; otherwise 0.
 */
int read_frame(int *fp, int channels, int bytes_per_sample)
{
    //int size = channels * bytes_per_sample; //how many bytes in total to read
    int channel = 0;
    int byte = 0;   //need to process each sample as a set
                    //will use nested for-loops

    for(channel = 0; channel<channels;channel++)
    {
        int value = 0;
        for(byte = 0;byte<bytes_per_sample;byte++)
        {
            char c = getchar();
           /* if(c==EOF)
            {
                printf("eof");
                return 0;
            }*/
            value = value<<8;
            value = value+c;
        }
        *(fp+channel) = value;
        //printf("value:%d\n",value);

    }

    return 1;
}



/**
 * @brief  Write, to the standard output, a single frame of audio data having
 * a specified number of channels and bytes per sample.
 * @details  This function takes a pointer 'fp' to a buffer that contains
 * 'channels' values of type 'int', and it writes these data values to the
 * standard output using big-endian byte order, resulting in a total of
 * 'channels * bytes_per_sample' data bytes written.
 *
 * @param  fp  A pointer to the buffer that contains the sample values to
 * be written.
 * @param  channels  The number of channels.
 * @param  bytes_per_sample  The number of bytes per sample.
 * @return  1 if the complete frame was written without error; otherwise 0.
 */
int write_frame(int *fp, int channels, int bytes_per_sample)
{

        int i,k;


        for(i=0;i<channels;i++)
        {
            int value = *(fp+i);

            for(k=1;k<=bytes_per_sample;k++)
            {
                int temp = value>>(8*(bytes_per_sample-k));
                printf("%c",temp);
            }
              //move to next int

        }

    return 1;
}

