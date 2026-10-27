/*
 * blitzlibcheck - check Blitz Basic 2 library files (see blitzlib.h).
 *
 *   blitzlibcheck LIB.obj...
 *
 * Silent if everything is right (warnings are printed); exit status 1 if
 * a file is not a valid library file.
 */
#include <stdio.h>
#include "blitzlib.h"

int main(int argc, char **argv)
{
    int i, bad = 0;

    if (argc < 2) {
        fprintf(stderr, "usage: blitzlibcheck LIB.obj...\n");
        return 2;
    }
    for (i = 1; i < argc; i++) {
        struct loadfile lf;
        char err[200];

        if (load_read(argv[i], &lf, err, sizeof err)) {
            fprintf(stderr, "blitzlibcheck: %s: %s\n", argv[i], err);
            bad = 1;
            continue;
        }
        if (blitzlib_check(&lf, argv[i], stderr))
            bad = 1;
        load_free(&lf);
    }
    return bad;
}
