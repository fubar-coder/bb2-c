/*
 * hunkcheck - check the start of the first hunk of a load file (see
 * hunkcheck.h).
 *
 *   hunkcheck FILE OFFSET:HEXBYTES...      e.g. hunkcheck ted 0:6100 4:48e7
 *   hunkcheck --hunks LAYOUT FILE          e.g. hunkcheck --hunks code,bss exe
 *   hunkcheck --stored FILE                every CODE/DATA hunk stored whole
 *
 * Silent if everything is right; exit status 1 otherwise.
 */
#include <stdio.h>
#include <string.h>
#include "hunkcheck.h"

static const char usage[] =
    "usage: hunkcheck FILE OFFSET:HEXBYTES...\n"
    "       hunkcheck --hunks LAYOUT FILE\n"
    "       hunkcheck --stored FILE\n"
    "  check the start of the first hunk of a load file, or its hunks\n"
    "  (LAYOUT: code|data|bss[+chip],... e.g. code,bss,data+chip), or that\n"
    "  no CODE/DATA hunk is larger than its stored block\n";

int main(int argc, char **argv)
{
    struct loadfile lf;
    char err[200];
    int bad;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "--hunks")) {
        if (load_read(argv[3], &lf, err, sizeof err)) {
            fprintf(stderr, "hunkcheck: %s: %s\n", argv[3], err);
            return 1;
        }
        bad = hunk_layout(&lf, argv[2], argv[3], stderr);
        load_free(&lf);
        return bad;
    }
    if (argc == 3 && !strcmp(argv[1], "--stored")) {
        if (load_read(argv[2], &lf, err, sizeof err)) {
            fprintf(stderr, "hunkcheck: %s: %s\n", argv[2], err);
            return 1;
        }
        bad = hunk_stored(&lf, argv[2], stderr);
        load_free(&lf);
        return bad;
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
