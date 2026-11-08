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
#include "hunk.h"
#include "vasmlisting.h"
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

const char cmd_listing_map_usage[] =
    "usage: refcheck listing-map LISTING\n"
    "  the rows of a vasm listing with bytes, as TSV: address, file:line, bytes,\n"
    "  source\n";
const char cmd_listing_labels_usage[] =
    "usage: refcheck listing-labels LISTING\n"
    "  the labels of a vasm listing, as TSV: address, label, file:line\n";
const char cmd_listing_verify_usage[] =
    "usage: refcheck listing-verify LISTING EXE\n"
    "  every byte of a vasm listing against the first hunk of the load file EXE\n"
    "  (relocated longwords skipped)\n";

/* the listing of argv[1] after argc - 1 == nargs arguments; 2 usage, 1 not read */
static int read_listing(int argc, char **argv, int nargs, const char *usage, struct listing *l)
{
    char err[200];

    me = argv[0];
    if (argc - 1 != nargs) {
        fputs(usage, stderr);
        return 2;
    }
    if (listing_read(argv[1], l, err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", me, err);
        return 1;
    }
    return 0;
}

int cmd_listing_map(int argc, char **argv)
{
    struct listing l;
    int rc = read_listing(argc, argv, 1, cmd_listing_map_usage, &l);

    if (rc)
        return rc;
    listing_print_map(&l, stdout);
    listing_free(&l);
    return 0;
}

int cmd_listing_labels(int argc, char **argv)
{
    struct listing l;
    int rc = read_listing(argc, argv, 1, cmd_listing_labels_usage, &l);

    if (rc)
        return rc;
    rc = listing_print_labels(&l, stdout) ? 1 : 0;
    listing_free(&l);
    return rc;
}

int cmd_listing_verify(int argc, char **argv)
{
    struct listing l;
    int rc = read_listing(argc, argv, 2, cmd_listing_verify_usage, &l);

    if (rc)
        return rc;
    rc = verify(&l, argv[2]);
    listing_free(&l);
    return rc;
}
