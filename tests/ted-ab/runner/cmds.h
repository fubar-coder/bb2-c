/*
 * cmds.h - the subcommands of tedab (main.c), each with its usage text
 * (the name tedab.h is the library's).
 */
#ifndef TEDAB_CMDS_H
#define TEDAB_CMDS_H

int cmd_run(int argc, char **argv);
int cmd_lint(int argc, char **argv);
int cmd_info(int argc, char **argv);
int cmd_text(int argc, char **argv);
int cmd_cmp(int argc, char **argv);

extern const char cmd_run_usage[];
extern const char cmd_lint_usage[];
extern const char cmd_info_usage[];
extern const char cmd_text_usage[];
extern const char cmd_cmp_usage[];

#endif
