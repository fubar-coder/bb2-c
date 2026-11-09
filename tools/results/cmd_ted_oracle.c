/*
 * results ted-oracle - the oracle data of the TED tests written from a
 * fresh run of the original TED, keeping the note rows
 * (lib/oracletable/oracletable.h).
 *
 *   results ted-oracle NEW OLD OUT    NEW: the original's run, OLD: the file
 *
 * OUT may be OLD.  Exit status 0 = written (the report on stdout), 1 =
 * errors (nothing written, the report on stderr), 2 = wrong arguments or
 * a file that cannot be read or written.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oracletable.h"
#include "results.h"

const char cmd_ted_oracle_usage[] =
    "usage: results ted-oracle NEW OLD OUT\n"
    "Write the oracle data of the TED tests OUT (tests/data/ted-oracle.sha256)\n"
    "from the original TED's fresh run NEW and the file OLD (OUT may be OLD).\n"
    "Rows with a note \"#NNN: TEXT (TED 1.1: HASH)\" behind a tab are note rows\n"
    "(the port's hash after a fix): the row stays, and its note must name NEW's\n"
    "hash.\n";

static char *read_all(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n = 0;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
        s[n] = 0;
    else {
        free(s);
        s = NULL;
    }
    fclose(f);
    if (s && strlen(s) != (size_t)n) {  /* a 0 byte */
        free(s);
        return NULL;
    }
    return s;
}

int cmd_ted_oracle(int argc, char **argv)
{
    const char *me = argv[0]; /* "results ted-oracle", the prefix of the messages */
    char *fresh, *old, *out, *report;
    int rc;

    if (argc != 4) {
        fputs(cmd_ted_oracle_usage, stderr);
        return 2;
    }
    fresh = read_all(argv[1]);
    old = read_all(argv[2]);
    if (!fresh || !old) {
        fprintf(stderr, "%s: %s: cannot read it\n", me, fresh ? argv[2] : argv[1]);
        free(fresh);
        free(old);
        return 2;
    }
    rc = oracletable_merge(fresh, old, &out, &report);
    fputs(report, rc ? stderr : stdout);
    if (!rc) {
        FILE *f = fopen(argv[3], "wb");

        if (!f || fwrite(out, 1, strlen(out), f) != strlen(out) || fclose(f)) {
            fprintf(stderr, "%s: %s: cannot write it\n", me, argv[3]);
            rc = 2;
        }
    }
    free(fresh);
    free(old);
    free(out);
    free(report);
    return rc;
}
