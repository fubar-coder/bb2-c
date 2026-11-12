/*
 * results.h - the subcommands of results (main.cpp): each one is in
 * cmd_NAME.cpp (cd-update and cd-reference in cmd_cd.cpp) and adds itself
 * to the program.
 */
#ifndef RESULTS_H
#define RESULTS_H

#include "cli.hpp"

void cmd_match(CLI::App &app, int &status);
void cmd_prev(CLI::App &app, int &status);
void cmd_cd_update(CLI::App &app, int &status);
void cmd_cd_reference(CLI::App &app, int &status);
void cmd_ted_oracle(CLI::App &app, int &status);
void cmd_libs_compare(CLI::App &app, int &status);

#endif
