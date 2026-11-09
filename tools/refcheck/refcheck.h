/*
 * refcheck.h - the subcommands of refcheck (main.cpp): each one is in
 * cmd_NAME.cpp (the three listing-* in cmd_listing.cpp) and adds itself to
 * the program.
 */
#ifndef REFCHECK_H
#define REFCHECK_H

#include "cli.hpp"

void cmd_asm(CLI::App &app, int &status);
void cmd_labels(CLI::App &app, int &status);
void cmd_data(CLI::App &app, int &status);
void cmd_listing_map(CLI::App &app, int &status);
void cmd_listing_labels(CLI::App &app, int &status);
void cmd_listing_verify(CLI::App &app, int &status);

#endif
