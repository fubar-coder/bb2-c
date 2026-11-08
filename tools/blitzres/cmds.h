/*
 * cmds.h - the subcommands of blitzres (main.c), in cmd_res.c with their
 * usage texts (the name blitzres.h is the library's).
 */
#ifndef BLITZRES_CMDS_H
#define BLITZRES_CMDS_H

int cmd_create(int argc, char **argv);
int cmd_dump(int argc, char **argv);
int cmd_source(int argc, char **argv);

extern const char cmd_create_usage[];
extern const char cmd_dump_usage[];
extern const char cmd_source_usage[];

#endif
