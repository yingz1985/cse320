
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "version.h"
#include "global.h"
#include "gradedb.h"
#include "stats.h"
#include "read.h"
#include "write.h"
#include "normal.h"
#include "sort.h"
#include "report.h"
#include "error.h"


/*
 * Course grade computation program
 */

#define REPORT          0
#define COLLATE         1
#define FREQUENCIES     2
#define QUANTILES       3
#define SUMMARIES       4
#define MOMENTS         5
#define COMPOSITES      6
#define INDIVIDUALS     7
#define HISTOGRAMS      8
#define TABSEP          9
#define ALLOUTPUT      10
#define NONAMES        11
#define SORTBY         12
#define OUTPUT         13

static struct option_info {
        unsigned int val;
        char *name;
        char chr;
        int has_arg;
        char *argname;
        char *descr;
} option_table[] = {
 {REPORT,         "report",    'r',      no_argument, NULL,
                  "Process input data and produce specified reports."},
 {COLLATE,        "collate",   'c',      no_argument, NULL,
                  "Collate input data and dump to standard output."},
 {FREQUENCIES,    "freqs",     0,        no_argument, NULL,
                  "Print frequency tables."},
 {QUANTILES,      "quants",    0,        no_argument, NULL,
                  "Print quantile information."},
 {SUMMARIES,      "summaries", 0,        no_argument, NULL,
                  "Print quantile summaries."},
 {MOMENTS,        "stats",     0,        no_argument, NULL,
                  "Print means and standard deviations."},
 {COMPOSITES,     "comps",     0,        no_argument, NULL,
                  "Print students' composite scores."},
 {INDIVIDUALS,    "indivs",    0,        no_argument, NULL,
                  "Print students' individual scores."},
 {HISTOGRAMS,     "histos",    0,        no_argument, NULL,
                  "Print histograms of assignment scores."},
 {TABSEP,         "tabsep",    0,        no_argument, NULL,
                  "Print tab-separated table of student scores."},
 {ALLOUTPUT,      "all",       'a',      no_argument, NULL,
                  "Print all reports."},
 {NONAMES,        "nonames",   'n',      no_argument, NULL,
                  "Suppress printing of students' names."},
 {SORTBY,         "sortby",    'k',      required_argument, "key",
                  "Sort by {name, id, score}."},
 {OUTPUT,         "output",    'o',      required_argument, "outfile",
                  "Write output to file, rather than standard output."},
 {0,NULL,0,0,NULL,NULL}

};

/*
*   first-second: name of the long option
*   has_arg: whether or not this long option requires an argument e.g. required_argument
*
*
*/
#define NUM_OPTIONS (14)

static char short_options[NUM_OPTIONS] = {'\0'};
//"rcank:o:";
static struct option long_options[NUM_OPTIONS+1];

static void init_options() {
    for(unsigned int i = 0; i < NUM_OPTIONS+1; i++) {
        struct option_info *oip = &option_table[i];
        struct option *op = &long_options[i];
        op->name = oip->name;
        op->has_arg = oip->has_arg;
        op->flag = NULL;
        op->val = oip->val;
    }

}
/*
static void init_short_options()
{
    int count = 0;
    for(unsigned int i = 0; i < NUM_OPTIONS; i++)
    {
        struct option_info *oip = &option_table[i];
        char character = oip->chr;
        char arg = ':';
        int hasArg = oip->has_arg;
        if(character != 0)
        {
            short_options[count++] = character;
            if(hasArg)
                short_options[count++] = arg;


        }
    }


}*/

static int report, collate, freqs, quantiles, summaries, moments,
           scores, composite, histograms, tabsep, nonames,output;

static void usage();


void freeCourse(Course* course)
{
    free(course->number);
    free(course->title);
    if(course->professor!=NULL)
    {
        free(course->professor->surname);
        free(course->professor->name);
        free(course->professor);
    }
    Assignment* temp = course->assignments;   //free all assignments in course
    while(temp != NULL)
    {
        Assignment* nowFree =temp;
        free(temp->name);
        free(temp->atype);
        temp = temp->next;
        free(nowFree);

    }

    Section* sect = course->sections;
    while(sect!=NULL)
    {
        Section* freeSect = sect;
        if(sect->assistant !=NULL)
        {

            free(sect->assistant->name);
            free(sect->assistant->surname);
            free(sect->assistant);
        }

        free(sect->name);
        //free students
        Student* student= sect->roster;
        while(student!=NULL)
        {
            Student* freeSt = student;
            free(student->id);
            free(student->surname);
            free(student->name);
            Score* raw = student->rawscores; //free scores within sections -> students
            while(raw!=NULL)
            {
                Score* freeScore = raw;
               /* for(temp = raw->asgt;temp != NULL;)
                {
                    Assignment* nowFree =temp;
                    free(temp->name);
                    free(temp->atype);
                    temp = temp->next;
                    free(nowFree);

                }*/
                if(raw->code != NULL)
                    free(raw->code);
                raw = raw->next;
                free(freeScore);
            }
            Score* norm = student->normscores;
            while(norm!=NULL)
            {
                Score* freeScor = norm;
                if(norm->code !=NULL)
                    free(norm->code);
                norm = norm->next;
                free(freeScor);

            }


            student = student->next;
            free(freeSt);
        }
        sect = sect->next;
        free(freeSect);


    }

    free(course);

}

void freeStats(Stats* stat)
{
    Classstats* s = stat->cstats;

    while(s!=NULL)
    {
        Classstats* freeC = s;

        Freqs* freq = s->freqs;

        while(freq!=NULL)
        {
            Freqs* ffreq = freq;
            freq= freq->next;
            free(ffreq);

        }
        Sectionstats* section = s->sstats;
        while(section!=NULL)
        {
            Sectionstats* fsection = section;
            Freqs* f = section->freqs;
            while(f!=NULL)
            {
                Freqs* ffreq = f;
                f= f->next;
                free(ffreq);

            }

            section= section->next;
            free(fsection);

        }



        s = s->next;
        free(freeC);

    }
    free(stat);


}

int main(argc, argv)
int argc;
char *argv[];
{
        Course *c;
        Stats *s;
        extern int errors, warnings;
        int optval;
        int (*compare)() = comparename;
        FILE* outFile = stdout;

        fprintf(stderr, BANNER);
        init_options();

        int count = 0;
        for(unsigned int i = 0; i < NUM_OPTIONS+1; i++)
        {
            struct option_info *oip = &option_table[i];
            char character = oip->chr;
            char arg = ':';
            int hasArg = oip->has_arg;
            if(character)
            {
                short_options[count++] = character;
                if(hasArg)
                    short_options[count++] = arg;


            }
        }

        if(argc <= 1) usage(argv[0]);
        while(optind < argc) {
            if((optval = getopt_long_only(argc, argv, short_options, long_options, NULL)) != -1) {
                switch(optval) {
                case REPORT:
                case 'r': if(optind==2) report++;
                        else
                        {
                            fprintf(stderr, "'%s' must only appear as the first argument.\n\n",
                            option_table[REPORT].name);
                            usage(argv[0]);
                        }
                        break;  //stacking cases
                case COLLATE:
                case 'c' : if(optind==2) collate++;
                            else
                        {
                            fprintf(stderr, "'%s' must only appear as the first argument.\n\n",
                            option_table[COLLATE].name);
                            usage(argv[0]);
                        } break;
                case OUTPUT:
                case 'o' : output++;
                    if((outFile = fopen(optarg, "w")) == NULL)
                    {
                        error("Can't write file: %s\n", optarg);
                        usage(argv[0]);
                    }
                    break;
                case TABSEP: tabsep++; break;
                case NONAMES:
                case 'n':  nonames++; break;
                case SORTBY:
                case 'k':
                    if(!strcmp(optarg, "name"))
                        compare = comparename;
                    else if(!strcmp(optarg, "id"))
                        compare = compareid;
                    else if(!strcmp(optarg, "score"))
                        compare = comparescore;
                    else {
                        fprintf(stderr,
                                "Option '%s' requires argument from {name, id, score}.\n\n",
                                option_table[SORTBY].name);
                        usage(argv[0]);
                    }
                    break;

                case FREQUENCIES: freqs++; break;
                case QUANTILES: quantiles++; break;
                case SUMMARIES: summaries++; break;
                case MOMENTS: moments++; break;
                case COMPOSITES: composite++; break;
                case INDIVIDUALS: scores++; break;
                case HISTOGRAMS: histograms++; break;
                case ALLOUTPUT:
                case 'a':
                    freqs++; quantiles++; summaries++; moments++;
                    composite++; scores++; histograms++; tabsep++;
                    break;
                case '?':
                    usage(argv[0]);
                    break;
                default:
                    break;
                }
            } else {
                break;
            }
        }
        if(optind == argc) {
                fprintf(stderr, "No input file specified.\n\n");
                usage(argv[0]);
        }
        char *ifile = argv[optind];
        //printf("filename:%s\n",ifile);
        if(report == collate) {
                fprintf(stderr, "Exactly one of '%s' or '%s' is required.\n\n",
                        option_table[REPORT].name, option_table[COLLATE].name);
                usage(argv[0]);

        }

        fprintf(stderr, "Reading input data...\n");
        c = readfile(ifile);
        if(errors) {
           printf("%d error%s found, so no computations were performed.\n",
                  errors, errors == 1 ? " was": "s were");
           exit(EXIT_FAILURE);
        }

        fprintf(stderr, "Calculating statistics...\n");
        s = statistics(c);
        if(s == NULL) fatal("There is no data from which to generate reports.");
        normalize(c);
        composites(c);
        sortrosters(c, comparename);
        checkfordups(c->roster);

        if(collate) {
                fprintf(stderr, "Dumping collated data...\n");
                writecourse(outFile, c);
                freeCourse(c);
                freeStats(s);
                exit(errors ? EXIT_FAILURE : EXIT_SUCCESS);
        }
        sortrosters(c, compare);

        fprintf(stderr, "Producing reports...\n");
        reportparams(outFile, ifile, c); // c =course*   ifile = file
        if(moments) reportmoments(outFile, s);
        if(composite) reportcomposites(outFile, c, nonames);
        if(freqs) reportfreqs(outFile, s);
        if(quantiles) reportquantiles(outFile, s);
        if(summaries) reportquantilesummaries(outFile, s);
        if(histograms) reporthistos(outFile, c, s);
        if(scores) reportscores(outFile, c, nonames);
        if(tabsep) reporttabs(outFile, c);

        fprintf(stderr, "\nProcessing complete.\n");
        printf("%d warning%s issued.\n", warnings+errors,
               warnings+errors == 1? " was": "s were");

        freeCourse(c);
        freeStats(s);

        exit(errors ? EXIT_FAILURE : EXIT_SUCCESS);
}

void usage(name)
char *name;
{
        struct option_info *opt;

        fprintf(stderr, "Usage: %s [options] <data file>\n", name);
        fprintf(stderr, "Valid options are:\n");
        for(unsigned int i = 0; i < NUM_OPTIONS; i++) {
                opt = &option_table[i];
                char optchr[5] = {' ', ' ', ' ', ' ', '\0'};
                if(opt->chr)
                  sprintf(optchr, "-%c, ", opt->chr);
                char arg[32];
                if(opt->has_arg)
                    sprintf(arg, " <%.10s>", opt->argname);
                else
                    sprintf(arg, "%.13s", "");
                fprintf(stderr, "\t%s--%-10s%-13s\t%s\n",
                            optchr, opt->name, arg, opt->descr);
                opt++;
        }
        exit(EXIT_FAILURE);
}


