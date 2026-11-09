/*
 * refcheck listing-map, listing-labels, listing-verify - views of a vasm
 * listing (logic in lib/vasmlisting).
 *
 *   refcheck listing-map LISTING          TSV: address  file:line  bytes
 *                                         source (rows with bytes only)
 *   refcheck listing-labels LISTING       TSV: address  label  file:line
 *   refcheck listing-verify LISTING EXE   every listed byte against the
 *                                         first hunk of EXE (relocated
 *                                         longs skipped); exit status 1 if
 *                                         a row differs
 */
#include <stdio.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "hunk.h"
#include "vasmlisting.h"
}
#include "refcheck.h"

static const char *me; /* "refcheck listing-...", the prefix of the messages */

static int verify(const struct listing *l, const char *exe)
{
    struct loadfile lf;
    char err[200];
    size_t bad;

    if (load_read(exe, &lf, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, exe, err);
        return 1;
    }
    if (!lf.nhunks || lf.hunks[0].type != HUNK_CODE) {
        fprintf(stderr, "%s: %s: the first hunk is not code\n", me, exe);
        load_free(&lf);
        return 1;
    }
    bad = listing_verify(l, &lf.hunks[0], stdout);
    printf("verify: %zu listing rows, %zu differ\n", l->nrows, bad);
    load_free(&lf);
    return bad ? 1 : 0;
}

/* the listing PATH; 1 if it cannot be read */
static int read_listing(const std::string &path, struct listing *l)
{
    char err[200];

    if (listing_read(path.c_str(), l, err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", me, err);
        return 1;
    }
    return 0;
}

/* the subcommand NAME with the argument LISTING (and EXE for verify) */
static CLI::App *add(CLI::App &app, const char *name, const char *description,
                     const std::shared_ptr<std::string> &listing)
{
    CLI::App *sub = app.add_subcommand(name, description);
    sub->add_option("LISTING", *listing, "a vasm listing (vasm -L)")->required();
    return sub;
}

void cmd_listing_map(CLI::App &app, int &status)
{
    auto listing = std::make_shared<std::string>();
    CLI::App *sub = add(app, "listing-map",
                        "the rows of a vasm listing with bytes, as TSV: address, file:line, bytes, source",
                        listing);
    sub->footer("Exit status 0, 1 the listing cannot be read, 2 usage error.");
    sub->callback([sub, listing, &status] {
        const std::string name = cli::name(*sub);
        struct listing l;

        me = name.c_str();
        if ((status = read_listing(*listing, &l)))
            return;
        listing_print_map(&l, stdout);
        listing_free(&l);
        status = 0;
    });
}

void cmd_listing_labels(CLI::App &app, int &status)
{
    auto listing = std::make_shared<std::string>();
    CLI::App *sub = add(app, "listing-labels", "the labels of a vasm listing, as TSV: address, label, file:line",
                        listing);
    sub->footer("Exit status 0, 1 the listing cannot be read or written, 2 usage error.");
    sub->callback([sub, listing, &status] {
        const std::string name = cli::name(*sub);
        struct listing l;

        me = name.c_str();
        if ((status = read_listing(*listing, &l)))
            return;
        status = listing_print_labels(&l, stdout) ? 1 : 0;
        listing_free(&l);
    });
}

void cmd_listing_verify(CLI::App &app, int &status)
{
    auto listing = std::make_shared<std::string>();
    auto exe = std::make_shared<std::string>();
    CLI::App *sub = add(app, "listing-verify",
                        "every byte of a vasm listing against the first hunk of a load file "
                        "(relocated longwords skipped)",
                        listing);
    sub->add_option("EXE", *exe, "the load file")->required();
    sub->footer("Exit status 0, 1 a row differs or an input cannot be read, 2 usage error.");
    sub->callback([sub, listing, exe, &status] {
        const std::string name = cli::name(*sub);
        struct listing l;

        me = name.c_str();
        if ((status = read_listing(*listing, &l)))
            return;
        status = verify(&l, exe->c_str());
        listing_free(&l);
    });
}
