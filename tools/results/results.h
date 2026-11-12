/*
 * results.h - the subcommands of results (main.cpp): each one is in
 * cmd_NAME.cpp (cd-update and cd-reference in cmd_cd.cpp, the libs-* ones
 * in cmd_libs.cpp, fix-scope in cmd_fix_scope.cpp) and adds itself to
 * the program.
 */
#ifndef RESULTS_H
#define RESULTS_H

#include <stdio.h>
#include <functional>
#include <string>
#include "cli.hpp"

/* fn(err), its error messages on stderr, each line prefixed by "NAME: "
   (cmd_libs.cpp); the result of fn */
int prefixed(const std::string &name, const std::function<int(FILE *)> &fn);

void cmd_match(CLI::App &app, int &status);
void cmd_prev(CLI::App &app, int &status);
void cmd_cd_update(CLI::App &app, int &status);
void cmd_cd_reference(CLI::App &app, int &status);
void cmd_ted_oracle(CLI::App &app, int &status);
void cmd_libs_compare(CLI::App &app, int &status);
void cmd_libs_accept(CLI::App &app, int &status);
void cmd_libs_update(CLI::App &app, int &status);
void cmd_fix_scope(CLI::App &app, int &status);

#endif
