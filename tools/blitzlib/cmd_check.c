/*
 * blitzlib check - check Blitz Basic 2 library files (see blitzlib.h).
 *
 *   blitzlib check LIB.obj...
 *
 * Silent if everything is right; exit status 1 if a file is not a valid
 * library file.
 */
#include <stdio.h>
#include <string.h>
#include "blitzlib.h"
#include "cmds.h"

const char cmd_check_usage[] =
    "usage: blitzlib check LIB.obj...\n"
    "  check Blitz Basic 2 library files\n";

static const char *me; /* "blitzlib check", the prefix of the messages */

int cmd_check(int argc, char **argv)
{
    int i, bad = 0;

    me = argv[0];
    if (argc < 2) {
        fputs(cmd_check_usage, stderr);
        return 2;
    }
    for (i = 1; i < argc; i++) {
        struct loadfile lf;
        char err[200];

        if (load_read(argv[i], &lf, err, sizeof err)) {
            fprintf(stderr, "%s: %s: %s\n", me, argv[i], err);
            bad = 1;
            continue;
        }
        if (blitzlib_check(&lf, argv[i], stderr))
            bad = 1;
        load_free(&lf);
    }
    return bad;
}
