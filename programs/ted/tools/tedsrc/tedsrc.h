/*
 * tedsrc.h - the subcommands of tedsrc (main.c): each one is in
 * cmd_NAME.c with its function and its usage text.
 */
#ifndef TEDSRC_H
#define TEDSRC_H

int cmd_tok(int argc, char **argv);
int cmd_detok(int argc, char **argv);

extern const char cmd_tok_usage[];
extern const char cmd_detok_usage[];

#endif
