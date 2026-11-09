/*
 * cd.h - the body of corpusrun cd (cd.c), called by its subcommand
 * (cmd_cd.cpp).
 */
#ifndef CORPUSRUN_CD_H
#define CORPUSRUN_CD_H

/* the CD run of the pass file PASSFILE (the keys: the header of cd.c);
 * PROG is the prefix of the messages.  The exit status (README.md, "Exit
 * codes"). */
int corpusrun_cd(const char *prog, const char *passfile);

#endif
