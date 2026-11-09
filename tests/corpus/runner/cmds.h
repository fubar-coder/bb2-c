/*
 * cmds.h - the subcommands of corpusrun (main.cpp), in cmd_pass.cpp and
 * cmd_cd.cpp; each adds itself to the program (the name corpus.h is the
 * library's).
 */
#ifndef CORPUSRUN_CMDS_H
#define CORPUSRUN_CMDS_H

#include "cli.hpp"

void cmd_pass(CLI::App &app, int &status);
void cmd_cd(CLI::App &app, int &status);

#endif
