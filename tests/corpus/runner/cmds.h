/*
 * cmds.h - the subcommands of corpusrun (main.c), each with its usage text
 * (the name corpus.h is the library's).
 */
#ifndef CORPUSRUN_CMDS_H
#define CORPUSRUN_CMDS_H

int cmd_pass(int argc, char **argv);
int cmd_cd(int argc, char **argv);

extern const char cmd_pass_usage[];
extern const char cmd_cd_usage[];

#endif
