/*
 * cmds.h - the subcommands of tedab (main.cpp), in cmd_run.cpp (run,
 * lint) and cmd_dump.cpp (info, text, cmp); each adds itself to the
 * program (the name tedab.h is the library's).
 */
#ifndef TEDAB_CMDS_H
#define TEDAB_CMDS_H

#include "cli.hpp"

void cmd_run(CLI::App &app, int &status);
void cmd_lint(CLI::App &app, int &status);
void cmd_info(CLI::App &app, int &status);
void cmd_text(CLI::App &app, int &status);
void cmd_cmp(CLI::App &app, int &status);

#endif
