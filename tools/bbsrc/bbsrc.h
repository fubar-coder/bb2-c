/*
 * bbsrc.h - the subcommands of bbsrc (main.c): each one is in a cmd_*.c
 * with its function and its usage text.
 */
#ifndef BBSRC_H
#define BBSRC_H

int cmd_tokens(int argc, char **argv);          /* cmd_tokens.c */
int cmd_detok(int argc, char **argv);           /* cmd_detok.c */
int cmd_comment(int argc, char **argv);         /* cmd_comment.c */
int cmd_bb22(int argc, char **argv);            /* cmd_bb22.c */
int cmd_bb22_info(int argc, char **argv);
int cmd_ab3(int argc, char **argv);             /* cmd_ab3.c */
int cmd_ab3_cmp(int argc, char **argv);

extern const char cmd_tokens_usage[];
extern const char cmd_detok_usage[];
extern const char cmd_comment_usage[];
extern const char cmd_bb22_usage[];
extern const char cmd_bb22_info_usage[];
extern const char cmd_ab3_usage[];
extern const char cmd_ab3_cmp_usage[];

#endif
