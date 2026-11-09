/*
 * refcheck - the port against the originals: assembler routines (asm),
 * the labels of ted.s in the C sources (labels), the data of the C build
 * against the reference build (data) and the views of a vasm listing
 * (listing-map, listing-labels, listing-verify).  README.md here; the
 * subcommands are cmd_NAME.cpp.
 */
#include "cli.hpp"
#include "refcheck.h"

int main(int argc, char **argv)
{
    CLI::App app{"The port against the originals: assembler routines, labels, data, vasm listings.",
                 "refcheck"};
    int status = 0;

    app.require_subcommand(1);
    cmd_asm(app, status);
    cmd_labels(app, status);
    cmd_data(app, status);
    cmd_listing_map(app, status);
    cmd_listing_labels(app, status);
    cmd_listing_verify(app, status);
    return cli::run(app, argc, argv, status);
}
