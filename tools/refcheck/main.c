/*
 * refcheck - the port against the originals: assembler routines (asm),
 * the labels of ted.s in the C sources (labels), the data of the C build
 * against the reference build (data) and the views of a vasm listing
 * (listing-map, listing-labels, listing-verify).  README.md here; the
 * subcommands are cmd_NAME.c.
 */
#include "subcmd.h"
#include "refcheck.h"

static const struct subcmd table[] = {
    { "asm", cmd_asm, cmd_asm_usage },
    { "labels", cmd_labels, cmd_labels_usage },
    { "data", cmd_data, cmd_data_usage },
    { "listing-map", cmd_listing_map, cmd_listing_map_usage },
    { "listing-labels", cmd_listing_labels, cmd_listing_labels_usage },
    { "listing-verify", cmd_listing_verify, cmd_listing_verify_usage },
};

int main(int argc, char **argv)
{
    return subcmd_main("refcheck", table, sizeof table / sizeof table[0], argc, argv);
}
