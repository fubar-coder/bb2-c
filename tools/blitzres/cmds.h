/*
 * cmds.h - the subcommands of blitzres (main.cpp), in cmd_res.cpp (the
 * name blitzres.h is the library's).  Each adds itself to the program.
 */
#ifndef BLITZRES_CMDS_H
#define BLITZRES_CMDS_H

#include "cli.hpp"

void cmd_create(CLI::App &app, int &status);
void cmd_dump(CLI::App &app, int &status);
void cmd_source(CLI::App &app, int &status);

#endif
