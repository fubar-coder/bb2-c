/*
 * cmds.h - the subcommands of blitzlib (main.cpp), in cmd_biglib.cpp
 * (join, extract, numbers) and cmd_NAME.cpp (the name blitzlib.h is the
 * library's).  Each adds itself to the program.
 */
#ifndef BLITZLIB_CMDS_H
#define BLITZLIB_CMDS_H

#include "cli.hpp"

void cmd_join(CLI::App &app, int &status);
void cmd_extract(CLI::App &app, int &status);
void cmd_numbers(CLI::App &app, int &status);
void cmd_check(CLI::App &app, int &status);
void cmd_compare(CLI::App &app, int &status);
void cmd_decls(CLI::App &app, int &status);
void cmd_cmdref(CLI::App &app, int &status);

#endif
