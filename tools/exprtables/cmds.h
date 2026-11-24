/*
 * cmds.h - the subcommands of exprtables (main.cpp), each in cmd_NAME.cpp;
 * each adds itself to the program.
 */
#ifndef EXPRTABLES_CMDS_H
#define EXPRTABLES_CMDS_H

#include "cli.hpp"

void cmd_page(CLI::App &app, int &status);
void cmd_candidate(CLI::App &app, int &status);

#endif
