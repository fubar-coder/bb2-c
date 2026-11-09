/*
 * results.h - the subcommands of results (main.c): each one is in
 * cmd_NAME.c with its function and its usage text.
 */
#ifndef RESULTS_H
#define RESULTS_H

int cmd_match(int argc, char **argv);
int cmd_prev(int argc, char **argv);
int cmd_cd_update(int argc, char **argv);
int cmd_cd_reference(int argc, char **argv);
int cmd_ted_oracle(int argc, char **argv);

extern const char cmd_match_usage[];
extern const char cmd_prev_usage[];
extern const char cmd_cd_update_usage[];
extern const char cmd_cd_reference_usage[];
extern const char cmd_ted_oracle_usage[];

#endif
