/*
 * subcmd.h - the subcommand dispatch of the host programs
 * (PROGRAM SUB [OPTIONS] ARGS; docs/development.md, "Host programs").
 *
 * A program's main() is a table of its subcommands and one call of
 * subcmd_main().  Every subcommand's usage text is its own `-h` output:
 * it starts with "usage: PROGRAM NAME", further synopsis lines start with
 * seven spaces and "PROGRAM NAME", description lines with two spaces; it
 * ends with a newline.  The program's usage is the synopsis lines of the
 * table in order (of each usage the first line and the lines after it
 * that start with seven spaces), the "usage: " of every entry after the
 * first replaced by seven spaces, and a line that names PROGRAM SUB -h.
 *
 * subcmd_main():
 *   no argument, or an unknown name   the program usage on stderr, 2
 *   -h or --help alone                the program usage on stdout, 0
 *   SUB -h or SUB --help alone        SUB's usage on stdout, 0
 *   SUB ARGS...                       SUB's run(argc - 1, argv + 1), with
 *                                     argv[0] the string "PROGRAM SUB" (the
 *                                     prefix of its messages); its status
 * A table that breaks the rules above (a missing name, function or usage,
 * a usage that does not start with "usage: PROGRAM NAME", a name twice)
 * is reported on stderr and gives 1, whatever the arguments.
 */
#ifndef SUBCMD_H
#define SUBCMD_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct subcmd {
    const char *name;                  /* "compare": lower case, '-' between words */
    int (*run)(int argc, char **argv); /* argv[0] = "PROGRAM SUB" for messages */
    const char *usage;                 /* "usage: PROGRAM SUB ...\n" (see above) */
};

/* the dispatch, on stdout and stderr */
int subcmd_main(const char *program, const struct subcmd *table, size_t n, int argc, char **argv);

/* the same with the two streams given (the tests) */
int subcmd_main_to(const char *program, const struct subcmd *table, size_t n, int argc,
                   char **argv, FILE *out, FILE *err);

/* 0 if the table follows the rules; else 1 and one line on err */
int subcmd_check(const char *program, const struct subcmd *table, size_t n, FILE *err);

/* the program usage (the synopsis lines of all entries) on f */
void subcmd_usage(const char *program, const struct subcmd *table, size_t n, FILE *f);

#ifdef __cplusplus
}
#endif

#endif
