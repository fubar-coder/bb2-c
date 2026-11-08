/*
 * testinfo.h - the subcommands of testinfo (main.c): each one is
 * cmd_NAME.c with its function and its usage text.
 */
#ifndef TESTINFO_H
#define TESTINFO_H

int cmd_stats(int argc, char **argv);
int cmd_map(int argc, char **argv);
int cmd_select(int argc, char **argv);

extern const char cmd_stats_usage[];
extern const char cmd_map_usage[];
extern const char cmd_select_usage[];

#endif
