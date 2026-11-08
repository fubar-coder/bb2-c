/*
 * cdtable - the CD result table written from a fresh run, keeping the
 * note rows (lib/cdtable/cdtable.h).
 *
 *   cdtable update    NEW OLD OUT    NEW: the port's run, OLD: the table
 *   cdtable reference NEW OLD OUT    NEW: the original's run
 *
 * OUT may be OLD.  Exit status 0 = written (the report on stdout), 1 =
 * errors (nothing written, the report on stderr), 2 = wrong arguments or
 * a file that cannot be read or written.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdtable.h"

static void usage(FILE *f)
{
    fprintf(f, "usage: cdtable update|reference NEW OLD OUT\n"
               "Write the CD result table OUT from the fresh run NEW and the table OLD (OUT\n"
               "may be OLD).  Rows whose note starts with \"#NNN:\" and names the CD's result\n"
               "as \"(2.10: HEX)\" (the masked hash) or \"(2.10: error LINE)\" (refused at\n"
               "that line) are note rows: with \"update\" (NEW is the port's run) the note\n"
               "stays unless the port's result is the CD's again; with \"reference\" (NEW is\n"
               "the original's run) the row stays, and its note must name NEW's result.\n");
}

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

int main(int argc, char **argv)
{
    char *fresh, *old, *out, *report;
    enum cdtable_mode mode;
    int rc;

    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        usage(stdout);
        return 0;
    }
    if (argc != 5 || (strcmp(argv[1], "update") && strcmp(argv[1], "reference"))) {
        usage(stderr);
        return 2;
    }
    mode = strcmp(argv[1], "update") ? CDTABLE_REFERENCE : CDTABLE_UPDATE;
    fresh = read_all(argv[2]);
    old = read_all(argv[3]);
    if (!fresh || !old) {
        fprintf(stderr, "cdtable: cannot read %s\n", fresh ? argv[3] : argv[2]);
        free(fresh);
        free(old);
        return 2;
    }
    rc = cdtable_merge(mode, fresh, old, &out, &report);
    fputs(report, rc ? stderr : stdout);
    if (!rc) {
        FILE *f = fopen(argv[4], "wb");

        if (!f || fwrite(out, 1, strlen(out), f) != strlen(out) || fclose(f)) {
            fprintf(stderr, "cdtable: cannot write %s\n", argv[4]);
            rc = 2;
        }
    }
    free(fresh);
    free(old);
    free(out);
    free(report);
    return rc;
}
