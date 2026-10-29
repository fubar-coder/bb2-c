/*
 * hunkcheck - check the start of the first hunk of a load file (see
 * hunkcheck.h).
 *
 *   hunkcheck FILE OFFSET:HEXBYTES...      e.g. hunkcheck ted 0:6100 4:48e7
 *
 * Silent if everything is right; exit status 1 otherwise.
 */
#include <stdio.h>
#include <string.h>
#include "hunkcheck.h"

static const char usage[] =
    "usage: hunkcheck FILE OFFSET:HEXBYTES...\n"
    "  check the start of the first hunk of a load file\n";

int main(int argc, char **argv)
{
    struct loadfile lf;
    char err[200];
    int bad;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    if (argc < 3) {
        fputs(usage, stderr);
        return 2;
    }
    if (load_read(argv[1], &lf, err, sizeof err)) {
        fprintf(stderr, "hunkcheck: %s: %s\n", argv[1], err);
        return 1;
    }
    bad = hunk_check(&lf, argc - 2, argv + 2, argv[1], stderr);
    load_free(&lf);
    return bad;
}
