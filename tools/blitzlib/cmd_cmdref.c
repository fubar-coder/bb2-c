/*
 * blitzlib cmdref - the command table of a Blitz Basic 2 library file as JSON, the
 * data of the website's command reference (format: lib/cmdref/cmdref.h).
 *
 *   blitzlib cmdref [-o OUT] -group GROUP -name NAME LIB.obj
 *
 * Without -o the JSON goes to stdout.  Exit status 1 and no OUT if the
 * file is not a readable library file.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "cmdref.h"
#include "cmds.h"

const char cmd_cmdref_usage[] =
    "usage: blitzlib cmdref [-o OUT] -group GROUP -name NAME LIB.obj\n"
    "  the command table of a Blitz Basic 2 library file as JSON (the data of\n"
    "  the website's command reference, site/data/libraries/GROUP/NAME.json)\n";

static const char *me; /* "blitzlib cmdref", the prefix of the messages */

int cmd_cmdref(int argc, char **argv)
{
    const char *out = NULL, *group = NULL, *name = NULL, *opens;
    struct loadfile lf;
    struct blitzlib lib;
    char err[300];
    int i, rc = 1;
    FILE *f;

    me = argv[0];
    for (i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "-o"))
            out = argv[i + 1];
        else if (!strcmp(argv[i], "-group"))
            group = argv[i + 1];
        else if (!strcmp(argv[i], "-name"))
            name = argv[i + 1];
        else
            break;
    }
    if (i + 1 != argc || !group || !name) {
        fputs(cmd_cmdref_usage, stderr);
        return 2;
    }
    if (out)
        remove(out);                    /* no stale file if this run fails */
    if (load_read(argv[i], &lf, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, argv[i], err);
        return 1;
    }
    if (lf.nhunks != 1 || blitzlib_read(&lf.hunks[0], &lib, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, argv[i],
                lf.nhunks != 1 ? "not one hunk (not a library file)" : err);
        load_free(&lf);
        return 1;
    }
    if (cmdref_opens(&lib, &opens, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, argv[i], err);
        goto done;
    }
    f = out ? fopen(out, "w") : stdout;
    if (!f) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        goto done;
    }
    if (cmdref_write(f, &lib, group, name, opens) || (out ? fclose(f) : fflush(f))) {
        fprintf(stderr, "%s: %s: %s\n", me, out ? out : "stdout", strerror(errno));
        if (out)
            remove(out);
        goto done;
    }
    rc = 0;
done:
    blitzlib_free(&lib);
    load_free(&lf);
    return rc;
}
