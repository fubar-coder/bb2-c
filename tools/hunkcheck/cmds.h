/*
 * cmds.h - the subcommands of hunkcheck (main.c), in cmd_check.c with
 * their usage texts (the name hunkcheck.h is the library's).
 */
#ifndef HUNKCHECK_CMDS_H
#define HUNKCHECK_CMDS_H

int cmd_start(int argc, char **argv);
int cmd_layout(int argc, char **argv);
int cmd_stored(int argc, char **argv);

extern const char cmd_start_usage[];
extern const char cmd_layout_usage[];
extern const char cmd_stored_usage[];

#endif
