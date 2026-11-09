/*
 * cmds.h - the subcommands of hunkcheck (main.cpp), in cmd_check.cpp (the
 * name hunkcheck.h is the library's).  Each adds itself to the program.
 */
#ifndef HUNKCHECK_CMDS_H
#define HUNKCHECK_CMDS_H

#include "cli.hpp"

void cmd_start(CLI::App &app, int &status);
void cmd_layout(CLI::App &app, int &status);
void cmd_stored(CLI::App &app, int &status);

#endif
