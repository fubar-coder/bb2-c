/*
 * run.h - the body of tedab run and tedab lint (run.c), called by their
 * subcommands (cmd_run.cpp).
 */
#ifndef TEDAB_RUN_H
#define TEDAB_RUN_H

/* the pass file PASSFILE read, the scripts loaded, then the run (LINT 0)
 * or only the check of the scripts and their sources (LINT 1); PROG is
 * the prefix of the messages.  The exit status (README.md, "Exit
 * codes"). */
int tedab_start(const char *prog, const char *passfile, int lint);

#endif
