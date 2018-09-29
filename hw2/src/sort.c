
#include "gradedb.h"
#include "sort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"


/*
 * Sort the Class and Section Rosters
 */

Student *getnext(s)
Student *s;
{
        return(s->next);
}

Student *getcnext(s)
Student *s;
{
        return(s->cnext);
}

void setnext(s, n)
Student *s, *n;
{
        s->next = n;
}

void setcnext(s, n)
Student *s, *n;
{
        s->cnext = n;
}

void sortrosters(c, compare)
Course *c;
int compare();
{
        Section *s;
        c->roster = sortroster(c->roster, getcnext, setcnext, compare);
        for(s = c->sections; s != NULL; s = s->next)
                s->roster = sortroster(s->roster, getnext, setnext, compare);
}

Student *sortroster(s, gtnxt, stnxt, compare)
Student *s;
Student *gtnxt();
void stnxt();
int compare();
{
        int count, i;
        Student *student, **stab;

        student = s;
        count = 0;
        while(student != NULL) {     /* Count students */
                count++;
                student = gtnxt(student);
        }
        if(count == 0) return(NULL);
        if((stab = (Student **)malloc(count*sizeof(Student))) == NULL)
                warning("Not enough memory to perform sorting.");
        student = s;
        i = count;
        while(i--) {            /* Put students in table */
                stab[i] = student;
                student = gtnxt(student);
        }
        sorttable(stab, 0, count, compare);
        student = stab[0];
        for(i = 1; i < count; i++) {    /* Relink students in order */
                stnxt(student, stab[i]);
                student = stab[i];
        }
        stnxt(student, NULL);
        student = stab[0];
        free(stab);//free memory allocated
        return(student);
}

/*
 * Quicksort algorithm for sorting table of students.
 */

void sorttable(stab, low, high, compare)
Student **stab;
int low, high;
int compare();
{
        int middle;
        if(low == high) return;
        middle = partition(stab, low, high, compare);
        if(low+1 == high) return;
        sorttable(stab, low, middle, compare);
        sorttable(stab, middle, high, compare);
}

int partition(stab, low, high, compare)
Student **stab;
int low, high;
int compare();
{
        int l, h, c, alt;
        Student *temp, *pivot;
        pivot = stab[(low+high)/2];
        l = low;
        h = high;
        alt = 1;
        while(l < h) {
                c = compare(stab[l], pivot);
                if(c == 0) {
                        c = alt;
                        alt = -alt;
                }
                if(c > 0) { /* put on high end */
                        temp = stab[h-1];
                        stab[h-1] = stab[l];
                        stab[l] = temp;
                        h--;
                } else {        /* put on low end */
                        l++;
                }
        }
        return(l);
}

int comparename(s1, s2)
Student *s1, *s2;
{
        int c;
        c = strcmp(s1->surname, s2->surname);
        if(c != 0) return(c);
        else return(strcmp(s1->name, s2->name));
}

int compareid(s1, s2)
Student *s1, *s2;
{
        //int c;
        return(strcmp(s1->id, s2->id));
}

int comparescore(s1, s2)
Student *s1, *s2;
{
        if(s1->composite > s2->composite) return(-1);
        else if(s1->composite < s2->composite) return(1);
        else return(0);
}

void checkfordups(student)
Student *student;
{
        while(student != NULL && student->cnext != NULL) {
                if(!comparename(student, student->cnext))
                        warning("Duplicate entry for student: %s, %s.",
                                student->surname, student->name);
                student = student->cnext;
        }
}
