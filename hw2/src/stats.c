
/*
 * Compute Per-assignment Statistics
 */

#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include "error.h"
#include "global.h"
#include "gradedb.h"
#include "stats.h"
#include "allocate.h"

Stats *statistics(c)
Course *c;
{
        Stats *s;
        s = buildstats(c);              /* Build "stats" data structure */
        if(s == NULL) return(s);        /* No data! */
        do_links(c, s);                 /* Fill in pointers */
        do_freqs(c);                    /* Count valid scores */
        do_quantiles(s);                /* Fill in quantile information */
        do_sums(c);                     /* Prepare to compute moments */
        do_moments(s);                  /* Now compute moments */
        return(s);
}

Stats *buildstats(c)
Course *c;
{
        Stats *stats;
        Classstats *classStat;
        Sectionstats *ssp;
        Assignment *ap;
        Section *sp;

        stats = newstats();
        stats->cstats = NULL;
        classStat = NULL;
        for(ap = c->assignments; ap != NULL; ap = ap->next) {
                if(classStat == NULL) {
                        stats->cstats = newclassstats();
                        classStat = stats->cstats;
                } else {
                        classStat->next = newclassstats();
                        classStat = classStat->next;
                }
                classStat->next = NULL;
                classStat->asgt = ap;
                classStat->valid = 0;
                classStat->tallied = 0;
                classStat->sum = classStat->sumsq = 0.0;
                classStat->freqs = NULL;
                classStat->sstats = NULL;
                ssp = NULL;
                for(sp = c->sections; sp != NULL; sp = sp->next) {
                        if(ssp == NULL) {
                                classStat->sstats = newsectionstats();
                                ssp = classStat->sstats;

                        } else {
                                ssp->next = newsectionstats();
                                ssp = ssp->next;
                        }
                        ssp->next = NULL;
                        ssp->asgt = ap;
                        ssp->section = sp;
                        ssp->valid = 0;
                        ssp->tallied = 0;
                        ssp->sum = ssp->sumsq = 0.0;
                        ssp->freqs = NULL;
                }
        }
        return(stats);
}

/*
 * Fill in pointers to make it easier to do stuff.
 *
 *      For each assignment in the course,
 *       for each section
 *        for each student in that section,
 *              link students into course roster.
 *         for each score for that student,
 *              link score to class and section statistics.
 */

void do_links(c, s)
Course *c;
Stats *s;
{
        Assignment *ap;
        Section *sep;
        Student *student, *rp;
        Score *score;

        Classstats *classStat;
        Sectionstats *ssp;

        classStat = s->cstats;
        c->roster = rp = NULL;
        for(ap = c->assignments; ap != NULL; ap = ap->next, classStat = classStat->next) {
           ssp = classStat->sstats;
           for(sep = c->sections; sep != NULL; sep = sep->next, ssp = ssp->next) {
              for(student = sep->roster; student != NULL; student = student->next) {
                 if(rp == NULL) {       /* Link students into course roster */
                        c->roster = student;
                        student->cnext = NULL;
                        rp = student;
                 } else {
                        rp->cnext = student;
                        student->cnext = NULL;
                        rp = student;
                 }
                 for(score = student->rawscores; score != NULL; score = score->next) {
                    if(score->asgt == ap) {
                       score->cstats = classStat; /* link scores to statistics for */
                       score->sstats = ssp; /* later use in normalization */
                    }
                 }
              }
           }
        }
}

/*
 * Construct frequency tables:
 *      For each student in the course roster,
 *        for each score for that student,
 *          if that score is a valid one, or if it has USERAW substitution,
 *               insert it into the class frequency tables for the
 *               appropriate assignment, and also into the section
 *               frequency tables for the appropriate assignment and section.
 */

void do_freqs(c)
Course *c;
{
        Student *student;
        Score *score;

        for(student = c->roster; student != NULL; student = student->cnext) {
           for(score = student->rawscores; score != NULL; score = score->next) {
               if(score->flag == VALID || score->subst == USERAW) {
                  score->cstats->freqs = count_score(score, score->cstats->freqs);
                  score->cstats->tallied++;
                  score->sstats->freqs = count_score(score, score->sstats->freqs);
                  score->sstats->tallied++;
               }
           }
        }
}

/*
 * Count the given score either by incrementing a count in an existing
 * bucket, or else inserting a new bucket with a count of one.
 * The head of the new list is returned.
 */

Freqs *count_score(score, afp)
Score *score;
Freqs *afp;
{
        Freqs *fp = NULL, *sfp = NULL;

        for(fp = afp; fp != NULL; sfp = fp, fp = fp->next) {

                if(fp->score == score->grade) {
                        fp->count++;
                        return(afp);
                } else if(fp->score > score->grade) {
                        if(sfp == NULL) {       /* insertion at head */
                                sfp = newfreqs();
                                sfp->next = fp;
                                sfp->score = score->grade;
                                sfp->count = 1;
                                return(sfp);    /* return new head */
                        } else {                /* insertion in middle */
                                sfp->next = newfreqs();
                                sfp = sfp->next;
                                sfp->next = fp;
                                sfp->score = score->grade;
                                sfp->count = 1;
                                return(afp);    /* return old head */
                        }
                } else continue;
        }
        if(sfp == NULL) {       /* insertion into empty list */
                sfp = newfreqs();
                sfp->next = NULL;
                sfp->score = score->grade;
                sfp->count = 1;
                return(sfp);    /* return new head */

        } else {                /* insertion at end of list */
                sfp->next = newfreqs();
                sfp = sfp->next;
                sfp->next = NULL;
                sfp->score = score->grade;
                sfp->count = 1;
                return(afp);
        }
}

/*
 * Traverse all the frequency tables and sum up counts to produce
 * quantile information.  This saves time in score normalization.
 */

void do_quantiles(s)
Stats *s;
{
        Classstats *classStat;
        Sectionstats *ssp;
        Freqs *fp;
        int sum;

        for(classStat = s->cstats; classStat != NULL; classStat = classStat->next) {
           sum = 0;
           for(fp = classStat->freqs; fp != NULL; fp = fp->next) {
              fp->numless = sum;
              sum += fp->count;
              fp->numlesseq = sum;
           }
           for(ssp = classStat->sstats; ssp != NULL; ssp = ssp->next) {
              sum = 0;
              for(fp = ssp->freqs; fp != NULL; fp = fp->next) {
                fp->numless = sum;
                sum += fp->count;
                fp->numlesseq = sum;
              }
           }
        }
}

/*
 * Compute sums necssary for determining moments:
 *      For each student in the course,
 *       for each score for that student,
 *            if that score is a valid one for the considered assigment,
 *              or if it has USERAW substitution,
 *            incorporate it into the sums for the whole class and
 *            for the section.
 */

void do_sums(c)
Course *c;
{
        Student *student;
        Score *score;
        Classstats *classStat;
        Sectionstats *ssp;
        double g;

        for(student = c->roster; student != NULL; student = student->cnext) {
           for(score = student->rawscores; score != NULL; score = score->next) {
              if(score->flag == VALID || score->subst == USERAW) {
                classStat = score->cstats;
                ssp = score->sstats;
                g = score->grade;
                if(classStat->valid++ == 0) classStat->min = classStat->max = g;
                else {
                        if(g < classStat->min) classStat->min = g;
                        if(g > classStat->max) classStat->max = g;
                }
                if(ssp->valid++ == 0) ssp->min = ssp->max = g;
                else {
                        if(g < classStat->min) classStat->min = g;
                        if(g > classStat->max) classStat->max = g;
                }
                classStat->sum += g;
                ssp->sum += g;
                g = g*g;
                classStat->sumsq += g;
                ssp->sumsq += g;
              }
           }
        }
}

/*
 * Traverse the data structure and use the accumulated data
 * to fill in the moments and quantiles.
 */

void do_moments(Stats *sp)
{
        Classstats *classStat;
        Sectionstats *ssp;

        for(classStat = sp->cstats; classStat != NULL; classStat = classStat->next) {
           if(classStat->valid) {
                classStat->mean = classStat->sum/classStat->valid;
                if(classStat->valid == 1) {
                   warning("Too few scores for %s.", classStat->asgt->name);
                   classStat->stddev = 0.0;
                } else {
                   classStat->stddev = stddev(classStat->valid, classStat->sum, classStat->sumsq);
                }
           } else {
                warning("No valid scores for %s.", classStat->asgt->name);
                classStat->mean = 0.0;
                classStat->stddev = 0.0;
           }
           for(ssp = classStat->sstats; ssp != NULL; ssp = ssp->next) {
              if(ssp->valid) {
                 ssp->mean = ssp->sum/ssp->valid;
                 if(ssp->valid == 1) {
                        warning("Too few scores for %s, section %s.",
                                ssp->asgt->name, ssp->section->name);
                        ssp->stddev = 0.0;
                 } else {
                    ssp->stddev = stddev(ssp->valid, ssp->sum, ssp->sumsq);
                 }
              } else {
                 warning("No valid scores for %s, section %s.",
                         ssp->asgt->name, ssp->section->name);
                 ssp->mean = 0.0;
                 ssp->stddev = 0.0;
              }
           }
        }
}

double stddev(n, sum, sumsq)
int n;
double sum;
double sumsq;
{
        if(n >= 2) return(sqrt((sumsq - (sum*sum)/n)/(n-1)));
        else return(0.0);
}
