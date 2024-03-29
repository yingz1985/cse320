

/*
 * Read in grade database from ASCII files
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "gradedb.h"
#include "stats.h"
#include "allocate.h"
#include "read.h"
#include "error.h"

/*
 * Input file stack
 */

Ifile *ifile;

/*
 * Token readahead buffer
 */


/*char tokenbuf[100];
char *tokenptr = tokenbuf;
char *tokenend = tokenbuf;
*/

char *tokenbuf;
char *tokenptr;
char *tokenend;
int size = 30;




//int istoken();
//int checktoken(char* key);

void init_Buffer()
{
    tokenbuf = malloc(sizeof(char) * size) ;
    tokenptr = tokenbuf;
    tokenend = tokenbuf;
}
void allocateMem()
{
    int cursor = tokensize();
    tokenbuf = realloc(tokenbuf,sizeof(char)*2*size);
    tokenptr = tokenbuf;
    tokenend = tokenptr+cursor;
    size = size*2;


}

Course *readfile(root)
char *root;
{
        init_Buffer();
        Course *c;

        ifile = newifile(); //allocate space for the new file
        memset(ifile,0,sizeof(Ifile));
        ifile->prev = NULL;
        ifile->name = root;
        ifile->line = 1;
        if((ifile->fd = fopen(root, "r")) == NULL)
        {

            free(ifile);
            free(tokenbuf);
            fatal("Can't open data file %s.\n", root);

        }

        fprintf(stderr, "[ %s", root);
        gobbleblanklines();
        //printf("gobbled lines");
        c = readcourse();
        gobbleblanklines();
        expecteof();
        fclose(ifile->fd);
        free(ifile);
        fprintf(stderr, " ]\n");
        free(tokenbuf);
        /*while(ifile!=NULL)
        {
            Ifile * temp = ifile;

            free(ifile->name);
            fclose(ifile->fd);
            ifile = ifile->prev;
            free(temp);
        }*/

        return(c);
}

Course *readcourse()
{
        Course *c;
        expecttoken("COURSE");
        //fprintf(stderr,"course read begin");
        c = newcourse();
        memset(c,0,sizeof(Course));
        //fprintf(stderr,"new course initialized\n");
        c->number = readid();
        //fprintf(stderr,"id read:%s\n",c->number);
        c->title = readname();
        //fprintf(stderr,"name read:%s\n",c->title);
        c->professor = readprofessor();
        //fprintf(stderr,"prof read:%s,%s\n",c->professor->surname,c->professor->name);
        c->assignments = readassignments();
        //fprintf(stderr,"assignments read\n");
        c->sections = readsections(c->assignments);
        //fprintf(stderr,"read course end");
        return(c);
}

Professor *readprofessor()
{
        Professor *p;
        if(!checktoken("PROFESSOR")) return(NULL);
        p = newprofessor();
        memset(p,0,sizeof(Professor));
        p->surname = readsurname();
        p->name = readname();
        return(p);
}

Assistant *readassistant()
{
        Assistant *a;
        if(!checktoken("ASSISTANT")) return(NULL);
        //fprintf(stderr,"token correct\n");
        a = newassistant();
        memset(a,0,sizeof(Assistant));
        //fprintf(stderr,"allocated space for new assistant\n");
        a->surname = readsurname();
        //fprintf(stderr,"surname read");
        a->name = readname();
        //fprintf(stderr,"\nread assistant");
        return(a);
}

Assignment *readassignments()
{
        Assignment *a;
        if(!checktoken("ASSIGNMENT")) return(NULL);
        a = newassignment();
        memset(a,0,sizeof(Assignment));
        a->name = readid();
        a->atype = readatype();
        expectnewline();
        a->wpolicy = NOWEIGHT;
        a->npolicy = RAW;
        a->weight = 0.0;
        a->max = 0.0;
        a->ngroup = BYCLASS;
        do {
                if(checktoken("WEIGHT")) {
                        readweight(a);
                        expectnewline();
                } else if(checktoken("NORMALIZE")) {
                        readnorm(a);
                        expectnewline();
                } else if(checktoken("MAXIMUM")) {
                        readmax(a);
                        expectnewline();
                } else break;
        } while(TRUE);
        a->next = readassignments();
        return(a);
}

Section *readsections(a)
Assignment *a;
{
        Section *s;
        while(!istoken()) {
                advancetoken();
                //fprintf(stderr,"advanced token\n");
                if(!istoken()) {
                        //fprintf(stderr,"not token\n");
                        if(ifile->prev == NULL) return(NULL);
                        else previousfile();
                }
        }
        if(checktoken("FILE")) {
                //fprintf(stderr,"found a new file\n");
                pushfile();
                //fprintf(stderr,"pushed file\n");
                return(readsections(a));
        }
        if(!checktoken("SECTION")) return(NULL);

        s = newsection();
        memset(s,0,sizeof(Section));

        s->name = readname();
        //fprintf(stderr,"read SECTION name:%s\n",s->name);
        s->assistant = readassistant();
        //fprintf(stderr,"read assistant name\n");
        s->roster = readstudents(a, s);
        //fprintf(stderr,"read students \n");
        s->next = readsections(a);
        //fprintf(stderr,"read students names\n");
        return(s);
}

Student *readstudents(a, sep)
Assignment *a;
Section *sep;
{
        Student *s;
        if(!checktoken("STUDENT")) return(NULL);
        s = newstudent();
        memset(s,0,sizeof(Student));
        s->id = readid();
        s->surname = readsurname();
        s->name = readname();
        s->rawscores = readscores(a);
        s->section = sep;
        s->next = readstudents(a, sep);
        return(s);
}


Score *readscores(a)
Assignment *a;
{
        Score *s;
        Assignment *ap;

        if(!checktoken("SCORE")) return(NULL);
        s = newscore();
        memset(s,0,sizeof(Score));
        if(!istoken()) advancetoken();
        s->asgt = NULL;
        /*
         * Failure to read assignment ID has to be a fatal error,
         * because other code depends on a valid s->asgt pointer.
         */
        if(istoken()) {
                for(ap = a; ap != NULL; ap = ap->next) {
                        if(!strcmp(ap->name, tokenptr)) {
                                s->asgt = ap;
                                break;
                        }
                }
                if(s->asgt == NULL)
                   fatal("(%s:%d) Undeclared assignment %s encountered in student scores.",
                         ifile->name, ifile->line, tokenptr);
                flushtoken();
        } else {
                fatal("(%s:%d) Expecting assignment ID.", ifile->name, ifile->line);
        }
        readgrade(s);
        expectnewline();
        s->next = readscores(a);
        return(s);
}

void readgrade(s)
Score *s;
{
        float f;

        s->flag = INVALID;
        if(checktoken("USERAW")) s->subst = USERAW;
        else if(checktoken("USENORM")) s->subst = USENORM;
        else if(checktoken("USELIKEAVG")) s->subst = USELIKEAVG;
        else if(checktoken("USECLASSAVG")) s->subst = USECLASSAVG;
        else s->flag = VALID;

        if(s->flag == VALID) {
                if(!istoken()) advancetoken();
                if(istoken() && (sscanf(tokenptr, "%f", &f) == 1)) {
                        flushtoken();
                        s->grade = f;
                } else {
                        error("(%s:%d) Expected a numeric score.", ifile->name, ifile->line);
                        s->grade = 0.0;
                }
                if(s->asgt->max != 0.0 && s->grade > s->asgt->max)
                  warning("(%s:%d) Grade (%f) exceeds declared maximum value (%f).\n",
                          ifile->name, ifile->line, s->grade, s->asgt->max);
        } else {
                switch(s->subst) {
                case USERAW:
                        if(!istoken()) advancetoken();
                        if(istoken() && (sscanf(tokenptr, "%f", &f) == 1)) {
                                flushtoken();
                                s->grade = f;
                        } else {
                                error("(%s:%d) Expected a numeric value.", ifile->name, ifile->line);
                                s->grade = 0.0;
                        }
                        if(s->asgt->max != 0.0 && s->grade > s->asgt->max)
                          warning("(%s:%d) Grade (%f) exceeds declared maximum value (%f).\n",
                                  ifile->name, ifile->line, s->grade, s->asgt->max);
                        break;
                case USENORM:
                        if(!istoken()) advancetoken();
                        if(istoken() && (sscanf(tokenptr, "%f", &f) == 1)) {
                          flushtoken();
                          s->qnorm = s->lnorm = s->snorm = f;
                          switch(s->asgt->npolicy) {
                          case RAW:
                            break;
                          case LINEAR:
                            if (s->lnorm < s->asgt->mean - 4.0*s->asgt->stddev ||
                                s->lnorm > s->asgt->mean + 4.0*s->asgt->stddev)
                              warning("(%s:%d) LINEAR score (%f) seems strange.\n",
                                      ifile->name, ifile->line, s->lnorm);
                            break;
                          case QUANTILE:
                            if(s->qnorm < 0.0 || s->qnorm > 100.0) {
                              error("(%s:%d) QUANTILE score (%f) not in [0.0, 100.0]\n",
                                    ifile->name, ifile->line, s->qnorm);
                            }
                            break;
                          case SCALE:
                            if(s->snorm < 0.0 || s->snorm > s->asgt->scale) {
                              error("(%s:%d) SCALE score (%f) not in [0.0, %f]\n",
                                    ifile->name, ifile->line, s->snorm, s->asgt->scale);
                            }
                            break;
                          }
                        } else {
                                error("(%s:%d) Expected a normalized score.", ifile->name, ifile->line);
                                s->qnorm = s->lnorm = s->snorm = 0.0;
                        }
                        break;
                case USELIKEAVG:
                case USECLASSAVG:
                        break;
                }
                advanceeol();
                s->code = newstring(tokenptr, tokensize());
                flushtoken();
        }
}

void readweight(a)
Assignment *a;
{
        float f;
        advancetoken();
        if(istoken() && sscanf(tokenptr, "%f", &f) == 1) {
                flushtoken();
                a->wpolicy = WEIGHT;
                a->weight = f;
        } else {
                error("(%s:%d) Expected a numeric weight.", ifile->name, ifile->line);
                a->wpolicy = NOWEIGHT;
        }
}

void readmax(a)
Assignment *a;
{
        float f;
        advancetoken();
        if(istoken() && sscanf(tokenptr, "%f", &f) == 1) {
                flushtoken();
                a->max = f;
        } else {
                error("(%s:%d) Expected a numeric maximum score.", ifile->name, ifile->line);
        }
}

void readnorm(a)
Assignment *a;
{
        float f;
        int found;
        found = 0;
        do {
                if(checktoken("RAW")) {
                        a->npolicy = RAW;
                } else if(checktoken("STDLINEAR")) {
                        a->npolicy = LINEAR;
                        a->mean = 0.0;
                        a->stddev = 1.0;
                } else if(checktoken("GENLINEAR")) {
                        a->npolicy = LINEAR;
                        advancetoken();
                        if(istoken() && sscanf(tokenptr, "%f", &f) == 1) {
                                flushtoken();
                                a->mean = f;
                        } else {
                                error("(%s:%d) Expected a numeric mean, using 0.0.", ifile->name, ifile->line);
                                a->mean = 0.0;
                        }
                        advancetoken();
                        if(istoken() && sscanf(tokenptr, "%f", &f) == 1) {
                                flushtoken();
                                a->stddev = f;
                        } else {
                                error("(%s:%d) Expected a numeric standard deviation, using 1.0.", ifile->name, ifile->line);
                                a->stddev = 1.0;
                        }
                } else if(checktoken("QUANTILE")) {
                        a->npolicy = QUANTILE;
                } else if(checktoken("SCALE")) {
                        a->npolicy = SCALE;
                        advancetoken();
                        if(istoken() && sscanf(tokenptr, "%f", &f) == 1) {
                                flushtoken();
                                a->scale = f;
                        } else {
                                error("(%s:%d) Expected a numeric scale, using 0.0.", ifile->name, ifile->line);
                                a->scale = 0.0;
                        }
                } else if(checktoken("BYCLASS")) {
                        a->ngroup = BYCLASS;
                } else if(checktoken("BYSECTION")) {
                        a->ngroup = BYSECTION;
                } else break;
                found++;
        } while(TRUE);
        if(!found) {
                a->npolicy = RAW;
                a->ngroup = BYCLASS;
                error("(%s:%d) Expected normalization information.", ifile->name, ifile->line);
        }
}

Surname readsurname()
{
        Surname s;
        if(!istoken()) advancetoken();
        if(istoken()) s = newstring(tokenptr, tokensize());
        else {
                error("(%s:%d) Expected surname.", ifile->name, ifile->line);
                s = newstring("", 0);
        }
        flushtoken();
        //fprintf(stderr,"surname:%s\n",s);
        return(s);
}

Name readname()
{
        Name n;
        advanceeol();

        if(istoken()) n = newstring(tokenptr, tokensize());
        else {
                error("(%s:%d) Expected a name.", ifile->name, ifile->line);
                n = newstring("", 0);
        }
        flushtoken();
        //fprintf(stderr,"name:%s\n",n);
        expectnewline();
        return(n);
}

Id readid()
{
        Id i;
        if(!istoken()) advancetoken();
        if(istoken()) i = newstring(tokenptr, tokensize());
        else {
                error("(%s:%d) Expected an ID.", ifile->name, ifile->line);
                i = newstring("", 0);
        }
        flushtoken();
        return(i);
}

Atype readatype()
{
        Atype a;
        if(!istoken()) advancetoken();
        if(istoken()) a = newstring(tokenptr, tokensize());
        else {
                error("(%s:%d) Expected an assignment type.", ifile->name, ifile->line);
                a = newstring("", 0);
        }
        flushtoken();
        return(a);
}

/*
 * See if there is read ahead in the token buffer
 */

int istoken()
{
        if(tokenptr != tokenend) return(TRUE);
        else return(FALSE);
}

/*
 * Determine the size of the token in the buffer
 */

int tokensize()
{
        return(tokenend-tokenptr);
}

/*
 * Flush the token readahead buffer
 */

void flushtoken()
{
        tokenptr = tokenend = tokenbuf;
        //restore the address
}

int iswhitespace(c)
char c;
{
        if(c == ',' || c == ':' || c == ' ' ||
                           c == '\t' || c == '\f') return(TRUE);
        else return(FALSE);
}

void gobblewhitespace()
{
        char c;
        if(istoken()) return;
        while(iswhitespace(c = getc(ifile->fd)));
        ungetc(c, ifile->fd);
}

void gobbleblanklines()
{
        int c;
        if(istoken()) return;
        do {

          if((c = getc(ifile->fd)) == '#') {
            while((c = getc(ifile->fd)) != '\n') {
              if(c == EOF)
                fatal("(%s:%d) EOF within comment line.",
                      ifile->name, ifile->line);
            }
            ifile->line++;
            continue;
          }
          //fprintf(stderr,"\nChar:%c\n",c);
          ungetc(c, ifile->fd);
          //fprintf(stderr,"putback: %c\n",k);

          while((c = getc(ifile->fd)) == '\n') {
            ifile->line++;
            gobblewhitespace();
            break;
          }
          //fprintf(stderr,"char%c:",c);
          if(c == '\n') continue;
          ungetc(c, ifile->fd);
          //fprintf(stderr,"charM:%c\n",m);
          return;
        } while(1);
}

/*
 * Return the next character, either from the token readahead buffer,
 * or else from the input stream.  If we see EOF, it's an error.
 */

char nextchar()
{
        int c;
        if(istoken()) return(*tokenptr++);
        flushtoken();
        if((c = getc(ifile->fd)) == EOF)
           fatal("(%s:%d) Unexpected EOF.", ifile->name, ifile->line);
        return(c);
}

/*
 * Read the next input token into the token readahead buffer.
 * If we already have a partial token in the buffer, it is an error
 */

void advancetoken()
{
        int c;
        if(istoken()) error("(%s:%d) Flushing unread input token.", ifile->name, ifile->line);
        flushtoken();
        gobblewhitespace();
        while((c = getc(ifile->fd)) != EOF) {
                if(iswhitespace(c) || c == '\n') {
                        ungetc(c, ifile->fd);
                        break;
                }
                if(tokensize() == size)
                    allocateMem();
                *tokenend++ = c;
        }
        if(tokenend != tokenptr)
        {
            if(tokensize() == size)
                    allocateMem();
            *tokenend++ = '\0';
        }
}

/*
 * Read from the current position to the end of the line into the
 * token buffer.  If we already had a token in the buffer, it's an error.
 */

void advanceeol()
{
        int c;
        if(istoken()) error("(%s:%d) Flushing unread input token.", ifile->name, ifile->line);
        flushtoken();

        gobblewhitespace();
        while((c = getc(ifile->fd)) != EOF) {
               // fprintf(stderr,"%c:",c);
                if(c == '\n') {
                        ungetc(c, ifile->fd);
                        break;
                }

                if(tokensize() == size)
                    allocateMem();
                *tokenend++ = c;

                //fprintf(stderr,"%c\n:",*(tokenend-1));
        }
        if(c == EOF)
                fatal("(%s:%d) Incomplete line at end of file.", ifile->name, ifile->line);
        if(tokensize() == size)
                    allocateMem();
        *tokenend++ = '\0';
}

/*
 * Check to see that the next token in the input matches a given keyword.
 * If it does not match, it is an error, and the input is left unchanged.
 * If it does match, the matched token is removed from the input stream.
 */

void expecttoken(key)
char *key;
{
        if(!istoken()) advancetoken();
        if(istoken() && !strcmp(tokenptr, key)) {
                flushtoken();
        } else {
                error("(%s:%d) Expected %s, found %s", ifile->name, ifile->line, key, tokenptr);
        }
}

/*
 * Check to see that the next token in the input matches a given keyword.
 * If it does not match, FALSE is returned, and the input is unchanged.
 * If it does match, the matched token is removed from the input stream,
 * and TRUE is returned.
 */

int checktoken(char* key)
{

        if(!istoken()) advancetoken();
        if(istoken() && !strcmp(tokenptr, key)) {
                flushtoken();
                return(TRUE);
        } else {
                return(FALSE);
        }
}

void expectnewline()
{
        char c;
        gobblewhitespace();
        if((c = nextchar()) == '\n') {
          ifile->line++;
          gobbleblanklines();
          return;
        }
        else {
                error("(%s:%d) Expected newline, scanning ahead.", ifile->name, ifile->line);
                flushtoken();
                while((c = nextchar()) != '\n');
                ifile->line++;
        }
}

void expecteof()
{
        int c;
        if(!istoken() && (c = getc(ifile->fd)) == EOF && ifile->prev == NULL)
           return;
        else {
                error("(%s:%d) Expected EOF, skipping excess input.", ifile->name, ifile->line);
                flushtoken();
                while(ifile->prev != NULL) previousfile();
        }
}

void previousfile()
{
        //fprintf(stderr,"prev file method entered\n");
        Ifile *prev;
        if((prev = ifile->prev) == NULL)
                fatal("(%s:%d) No previous file.", ifile->name, ifile->line);

        free(ifile->name);
        fclose(ifile->fd);
        //fprintf(stderr,"closed current file\n");
        //free(ifile->fd);
        free(ifile);
        //fprintf(stderr,"freed current file\n");
        //fclose(ifile->fd);
        //fprintf(stderr,"closed current file\n");
        ifile = prev;
        fprintf(stderr, " ]");

}
//initially with parameter int e
void pushfile()
{
        Ifile *nfile;
        char *n;

        advanceeol();
        if(istoken()) n = newstring(tokenptr, tokensize());
        else {
                error("(%s:%d) Expected a file name.", ifile->name, ifile->line);
                n = newstring("", 0);
        }
        flushtoken();
        expectnewline();

        nfile = newifile();
        nfile->prev = ifile;
        nfile->name = n;
        nfile->line = 1;
        if((nfile->fd = fopen(n, "r")) == NULL)
                fatal("(%s:%d) Can't open data file %s\n", ifile->name, ifile->line, n);
        ifile = nfile;
        fprintf(stderr, " [ %s", n);
        gobbleblanklines();
        //fprintf(stderr,"\nread new file and gobbled lines again\n");
}

