/*
 * ab3main.h - the command line run of blitz2-ab3 (ab3main.c, only in
 * blitz2-ab3; spec ab3-compat/command-line).
 */
#ifndef AB3MAIN_H
#define AB3MAIN_H

#include "ab3job.h"

/** @brief The job of a command line run; NULL in an editor session. */
extern struct ab3job *ab3job;

/** @brief The editor of a command line run (instead of blitz2:ted). */
extern const char ab3cliname[];

/**
 * @brief Read the command line; for a compile decide the program's mode
 *        (progmode) and set ab3job.
 *
 * Before anything is loaded (the libraries depend on the mode).  Prints
 * the usage and the messages of argument errors itself.
 *
 * @param argc  main's argc (> 0: started from the shell)
 * @param argv  main's argv
 * @return -1: go on (editor session, or the compile of ab3job); else the
 *         return code of the run (usage, argument or set-up error)
 */
int ab3_cmdline(int argc, char **argv);

struct include;

/**
 * @brief A text include in a command line run (task 3.5): blitz2 reads an
 *        include raw (readinc), so ab3cli tokenises it first, as TED loads
 *        a source, with the program's token list (job tokfile); readinc
 *        then reads those lines (ab3_incread) instead of the file.
 *
 * An error stops the compile (err).
 *
 * @param inc   the include, just opened (doinc2)
 * @param name  its name
 */
void ab3_textinc(struct include *inc, const char *name);

/**
 * @brief readinc's Read() of an include: the tokenised lines of a text
 *        include, else the file.
 * @return the bytes read, -1 on an error
 */
LONG ab3_incread(struct include *inc, UBYTE *dst, LONG len);

/** @brief The include is closed: its tokenised lines go. */
void ab3_incdone(struct include *inc);

/**
 * @brief The return code of a command line run, after the compiler ran
 *        (ab3cli left it in the job).
 * @return 0, 10 or 20
 */
int ab3_result(void);

#endif
