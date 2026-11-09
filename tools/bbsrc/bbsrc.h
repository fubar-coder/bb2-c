/*
 * bbsrc.h - the subcommands of bbsrc (main.cpp): each one is in a
 * cmd_*.cpp and adds itself to the program.
 */
#ifndef BBSRC_H
#define BBSRC_H

#include "cli.hpp"

void cmd_tokens(CLI::App &app, int &status);    /* cmd_tokens.cpp */
void cmd_detok(CLI::App &app, int &status);     /* cmd_detok.cpp */
void cmd_comment(CLI::App &app, int &status);   /* cmd_comment.cpp */
void cmd_bb22(CLI::App &app, int &status);      /* cmd_bb22.cpp */
void cmd_bb22_info(CLI::App &app, int &status);
void cmd_ab3(CLI::App &app, int &status);       /* cmd_ab3.cpp */
void cmd_ab3_cmp(CLI::App &app, int &status);

#endif
