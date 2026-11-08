/*
 * refcheck.h - the subcommands of refcheck (main.c): each one is in
 * cmd_NAME.c with its function and its usage text.
 */
#ifndef REFCHECK_H
#define REFCHECK_H

int cmd_asm(int argc, char **argv);
int cmd_labels(int argc, char **argv);
int cmd_data(int argc, char **argv);
int cmd_listing_map(int argc, char **argv);
int cmd_listing_labels(int argc, char **argv);
int cmd_listing_verify(int argc, char **argv);

extern const char cmd_asm_usage[];
extern const char cmd_labels_usage[];
extern const char cmd_data_usage[];
extern const char cmd_listing_map_usage[];
extern const char cmd_listing_labels_usage[];
extern const char cmd_listing_verify_usage[];

#endif
