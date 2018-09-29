
/*
 * Normalize scores, using the computed statistics.
 */

#include<stddef.h>
#include<stdio.h>
#include <string.h>
#include "global.h"
#include "gradedb.h"
#include "stats.h"
#include "allocate.h"
#include "normal.h"
#include "error.h"

/*
 * Normalize scores:
 *      For each student in the course roster,
 *       for each score for that student,
 *              compute the normalized version of that score
 *              according to the substitution and normalization
 *              options set for that score and for the assignment.
 */

void normalize(course)
Course *course;
//Stats *s;
{
        Student *student;
        Score *rawScore, *normScore;
        Classstats *classStats;
        Sectionstats *sectionStat;

        for(student = course->roster; student != NULL; student = student->cnext) {
           student->normscores = normScore = NULL;
           for(rawScore = student->rawscores; rawScore != NULL; rawScore = rawScore->next) {
              classStats = rawScore->cstats;  //statistics by class
              sectionStat = rawScore->sstats; //statistics by section
              if(normScore == NULL) {
                student->normscores = normScore = newscore();
                memset(normScore,0,sizeof(Score));
                normScore->next = NULL;
              } else {
                normScore->next = newscore();
                memset(normScore->next,0,sizeof(Score));
                normScore = normScore->next;
                normScore->next = NULL;
              }
              normScore->asgt = rawScore->asgt;
              normScore->flag = rawScore->flag;
              normScore->subst = rawScore->subst;
              if(rawScore->flag == VALID) {
                normScore->grade = normal(rawScore->grade, classStats, sectionStat);
              } else {
                switch(rawScore->subst) {
                case USERAW:
                        normScore->grade = normal(rawScore->grade, classStats, sectionStat);
                        break;
                case USENORM:
                        if(rawScore->asgt->npolicy == QUANTILE)
                                normScore->grade = rawScore->qnorm;
                        else
                                normScore->grade = rawScore->lnorm;
                        break;
                case USELIKEAVG:
                        normScore->grade = studentavg(student, classStats->asgt->atype);
                        break;
                case USECLASSAVG:
                        if(rawScore->asgt->npolicy == QUANTILE)
                                normScore->grade = 50.0;
                        else
                                normScore->grade = rawScore->asgt->mean;
                        break;
                }
              }
           }
        }
        //change made here ***
        //s->cstats = classStats;
        //****
}

/*
 * Normalize a raw score according to the normalization policy indicated.
 */

float normal(s, classStats, sectionStat)
double s;
Classstats *classStats;
Sectionstats *sectionStat;
{
        Assignment *a;
        Freqs *fp;
        int n;

        a = classStats->asgt;
        switch(a->npolicy) {
        case RAW:
                return(s);
        case LINEAR:
                switch(a->ngroup) {
                case BYCLASS:
                        if(classStats->stddev < EPSILON) {
                           warning("Std. dev. of %s too small for normalization.",
                                 classStats->asgt->name);
                           classStats->stddev = 2*EPSILON;
                         }
                        return(linear(s, classStats->mean, classStats->stddev, a->mean, a->stddev));
                case BYSECTION:
                        if(sectionStat->stddev < EPSILON) {
                           warning("Std. dev. of %s, section %s too small for normalization.",
                                 sectionStat->asgt->name, sectionStat->section->name);
                           sectionStat->stddev = 2*EPSILON;
                         }
                        return(linear(s, sectionStat->mean, sectionStat->stddev, a->mean, a->stddev));
                }
        case SCALE:
                if(a->max < EPSILON) {
                  warning("Declared maximum score of %s too small for normalization.",
                        classStats->asgt->name);
                  a->max = 2*EPSILON;
                }
                return(scale(s, a->max, a->scale));
        case QUANTILE:
                switch(a->ngroup) {
                case BYCLASS:
                        fp = classStats->freqs;
                        n = classStats->tallied;
                        if(n == 0) {
                           warning("Too few scores in %s for quantile normalization.",
                                   classStats->asgt->name);
                           n = 1;
                         }
                        break;
                case BYSECTION:
                        fp = sectionStat->freqs;
                        n = sectionStat->tallied;
                        if(n == 0) {
                           warning("Too few scores in %s, section %s for quantile normalization.",
                                 sectionStat->asgt->name, sectionStat->section->name);
                           n = 1;
                         }
                        break;
                }
                /*
                 * Look for score s in the frequency tables.
                 * If found, return the corresponding percentile score.
                 * If not found, then use the percentile score corresponding
                 * to the greatest valid score in the table that is < s.
                 */

                for( ; fp != NULL; fp = fp->next) {
                   if(s < fp->score)
                        return((float)fp->numless*100.0/n);
                   else if(s == fp->score)
                        return((float)fp->numless*100.0/n);
                }
        }
        return 0;
}

/*
 * Perform a linear transformation to convert score s,
 * with sample mean rm and sample standard deviation rd,
 * to a normalized score with normalized mean nm and
 * normalized standard deviation nd.
 *
 * It is assumed that rd is not too small.
 */

float linear(s, rm, rd, nm, nd)
double s, rm, rd, nm, nd;
{
        return(nd*(s-rm)/rd + nm);
}

/*
 * Scale normalization rescales the score to a given range [0, scale]
 *
 * It is assumed that the declared max is not too small.
 */

float scale(s, max, scale)
double s, max, scale;
{
        return(s*scale/max);
}

/*
 * Compute a student's average score on all assignments of a given type.
 * If a weighting policy is set, we use the specified relative weights,
 * otherwise all the assignments of the type are weighted equally.
 */

float studentavg(s, t)
Student *s;
Atype t;
{
        int n, wp;
        double sum;
        Score *scp;
        float f, w;

        n = 0;
        wp = 0;
        sum = 0.0;
        w = 0.0;
        for(scp = s->rawscores; scp != NULL; scp = scp->next) {
           if(!strcmp(scp->asgt->atype, t) &&
              (scp->flag == VALID || scp->subst == USERAW)) {
                n++;
                f = normal(scp->grade, scp->cstats, scp->sstats);
                if(scp->asgt->wpolicy == WEIGHT) {
                   wp = 1;
                   sum += f * scp->asgt->weight;
                   w += scp->asgt->weight;
                } else {
                   sum += f;
                }
           }
        }
        if(n == 0 || w == 0.0) {
                warning("Student %s %s has no like scores to average,\n%s",
                        s->name, s->surname, "using raw 0.0.");
                return(0.0);
        } else {
                if(wp) return(sum/w);
                else return(sum/n);
        }
}

/*
 *  Compute composite scores:
 *      For each student in the course roster,
 *        For each assignment,
 *         Find the student's normalized score for that assignment,
 *              and include it in the weighted sum.
 */

void composites(c)
Course *c;
{
        Student *student;
        Score *scp;
        Assignment *ap;
        float sum;
        int found;

        for(student = c->roster; student != NULL; student = student->cnext) {
           sum = 0.0;
           for(ap = c->assignments; ap != NULL; ap = ap->next) {
              found = 0;
              for(scp = student->normscores; scp != NULL; scp = scp->next) {
                if(scp->asgt == ap) {
                   found++;
                   sum += scp->grade * (ap->wpolicy == WEIGHT? ap->weight: 1.0);
                }
              }
              if(!found) {
                warning("Student %s %s has no score for assignment %s.",
                        student->name, student->surname, ap->name);
              }
           }
           student->composite = sum;
        }
}
