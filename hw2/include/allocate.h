/*
 * Type definitions for memory allocation functions
 */

Professor *newprofessor();
Assistant *newassistant();
Student *newstudent();
Section *newsection();
Assignment *newassignment();
Course *newcourse();
Score *newscore();
char *newstring();

Freqs *newfreqs();
Classstats *newclassstats();
Sectionstats *newsectionstats();
Stats *newstats();
Ifile *newifile();

extern char *memerr ;
//= "Unable to allocate memory.";
