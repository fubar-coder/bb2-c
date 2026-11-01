/*
 * ab3main.h - the command line run of blitz2-ab3 (ab3main.c, only in
 * blitz2-ab3; spec ab3-compat/command-line).
 */
#ifndef AB3MAIN_H
#define AB3MAIN_H

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

/**
 * @brief The return code of a command line run, after the compiler ran
 *        (ab3cli left it in the job).
 * @return 0, 10 or 20
 */
int ab3_result(void);

#endif
