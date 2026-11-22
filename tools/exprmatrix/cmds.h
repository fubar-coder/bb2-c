/*
 * cmds.h - the subcommands of exprmatrix (main.cpp), each in cmd_NAME.cpp;
 * each adds itself to the program.
 */
#ifndef EXPRMATRIX_CMDS_H
#define EXPRMATRIX_CMDS_H

#include "cli.hpp"

void cmd_generate(CLI::App &app, int &status);
void cmd_classify(CLI::App &app, int &status);
void cmd_expect(CLI::App &app, int &status);

#endif
