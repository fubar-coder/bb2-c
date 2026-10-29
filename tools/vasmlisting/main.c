/*
 * vasmlisting - views of a vasm listing (the former tedlisting.py of bb2;
 * logic in lib/vasmlisting).
 *
 *   vasmlisting LISTING map           TSV: address  file:line  bytes  source
 *                                     (rows with bytes only)
 *   vasmlisting LISTING labels        TSV: address  label  file:line
 *   vasmlisting LISTING verify EXE    every listed byte against the first
 *                                     hunk of EXE (relocated longs
 *                                     skipped); exit status 1 if a row
 *                                     differs
 */
#include <stdio.h>
#include <string.h>
#include "hunk.h"
#include "vasmlisting.h"

static int verify(const struct listing *l, const char *exe)
{
    struct loadfile lf;
    char err[200];
    size_t bad;

    if (load_read(exe, &lf, err, sizeof err)) {
        fprintf(stderr, "vasmlisting: %s: %s\n", exe, err);
        return 1;
    }
    if (!lf.nhunks || lf.hunks[0].type != HUNK_CODE) {
        fprintf(stderr, "vasmlisting: %s: the first hunk is not code\n", exe);
        load_free(&lf);
        return 1;
    }
    bad = listing_verify(l, &lf.hunks[0], stdout);
    printf("verify: %zu listing rows, %zu differ\n", l->nrows, bad);
    load_free(&lf);
    return bad ? 1 : 0;
}

static const char usage[] =
    "usage: vasmlisting LISTING map|labels|verify EXE\n"
    "  views of a vasm listing\n";

int main(int argc, char **argv)
{
    struct listing l;
    char err[200];
    int rc;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    if (argc < 3 || (strcmp(argv[2], "map") && strcmp(argv[2], "labels")
                     && (strcmp(argv[2], "verify") || argc < 4))) {
        fputs(usage, stderr);
        return 2;
    }
    if (listing_read(argv[1], &l, err, sizeof err)) {
        fprintf(stderr, "vasmlisting: %s\n", err);
        return 1;
    }
    if (!strcmp(argv[2], "map")) {
        listing_print_map(&l, stdout);
        rc = 0;
    } else if (!strcmp(argv[2], "labels")) {
        rc = listing_print_labels(&l, stdout) ? 1 : 0;
    } else {
        rc = verify(&l, argv[3]);
    }
    listing_free(&l);
    return rc;
}
