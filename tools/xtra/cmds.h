/*
 * cmds.h - the subcommands of xtra (main.cpp), in cmd_xtra.cpp; each adds
 * itself to the program (the name xtra.h is the library's).
 */
#ifndef XTRA_CMDS_H
#define XTRA_CMDS_H

#include "cli.hpp"

void cmd_dump(CLI::App &app, int &status);
void cmd_check(CLI::App &app, int &status);
void cmd_convert(CLI::App &app, int &status);

#endif
