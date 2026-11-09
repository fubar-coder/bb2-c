/*
 * cmds.h - the subcommands of hdftool (main.cpp), in cmd_hdf.cpp; each
 * adds itself to the program (the name hdf.h is the library's).
 */
#ifndef HDFTOOL_CMDS_H
#define HDFTOOL_CMDS_H

#include "cli.hpp"

void cmd_info(CLI::App &app, int &status);
void cmd_list(CLI::App &app, int &status);
void cmd_cat(CLI::App &app, int &status);
void cmd_check(CLI::App &app, int &status);
void cmd_edit(CLI::App &app, int &status);

#endif
