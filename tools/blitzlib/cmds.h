/*
 * cmds.h - the subcommands of blitzlib (main.c), each with its usage
 * text (the name blitzlib.h is the library's).
 */
#ifndef BLITZLIB_CMDS_H
#define BLITZLIB_CMDS_H

int cmd_join(int argc, char **argv);
int cmd_extract(int argc, char **argv);
int cmd_numbers(int argc, char **argv);
int cmd_check(int argc, char **argv);
int cmd_compare(int argc, char **argv);
int cmd_cmdref(int argc, char **argv);

extern const char cmd_join_usage[];
extern const char cmd_extract_usage[];
extern const char cmd_numbers_usage[];
extern const char cmd_check_usage[];
extern const char cmd_compare_usage[];
extern const char cmd_cmdref_usage[];

#endif
