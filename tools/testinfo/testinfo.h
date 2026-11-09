/*
 * testinfo.h - the subcommands of testinfo (main.cpp): each one is
 * cmd_NAME.cpp and adds itself to the program.
 */
#ifndef TESTINFO_H
#define TESTINFO_H

#include "cli.hpp"

void cmd_stats(CLI::App &app, int &status);
void cmd_map(CLI::App &app, int &status);
void cmd_select(CLI::App &app, int &status);

#endif
