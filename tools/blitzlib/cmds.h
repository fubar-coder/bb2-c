/*
 * cmds.h - the subcommands of blitzlib (main.cpp), in cmd_biglib.cpp
 * (join, extract, numbers) and cmd_NAME.cpp (the name blitzlib.h is the
 * library's).  Each adds itself to the program.
 */
#ifndef BLITZLIB_CMDS_H
#define BLITZLIB_CMDS_H

#include "cli.hpp"

void cmd_join(CLI::App &app, int &status);
void cmd_extract(CLI::App &app, int &status);
void cmd_numbers(CLI::App &app, int &status);
void cmd_check(CLI::App &app, int &status);
void cmd_compare(CLI::App &app, int &status);
void cmd_decls(CLI::App &app, int &status);
void cmd_cmdref(CLI::App &app, int &status);

/* the reservation of libraries 32 and 33 (#1018, blitzlib_reserved) for
   the library file in data/size (a hunk file); what names it in the
   messages; allow33: a library 33 other than literallib is allowed.
   0 = ok, 1 = refused (the message on stderr with prefix me) */
int check_reserved(const char *me, const unsigned char *data, size_t size, const char *what,
                   int allow33);

#endif
