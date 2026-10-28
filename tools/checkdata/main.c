/*
 * checkdata - data of a C build against the reference build of the
 * assembler original (see checkdata.h).
 *
 *   checkdata [-v|-q] TABLE LISTING REFERENCE CBUILD
 *   e.g. checkdata programs/ted/checkdata.tsv ref/ted.lst ref/ted ted.sym
 *
 * Prints the failed checks and a summary (-q: only if a check failed, -v:
 * every check); exit status 1 if a check failed.
 */
#define _XOPEN_SOURCE 700       /* open_memstream */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "checkdata.h"

static char *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
        s[n] = 0;
        *len = (size_t)n;
    } else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

int main(int argc, char **argv)
{
    int verbose = argc > 1 && !strcmp(argv[1], "-v");
    int quiet = argc > 1 && !strcmp(argv[1], "-q");
    int opt = verbose || quiet;
    struct checkdata_table table;
    struct listing listing;
    struct loadfile ref, c;
    char err[200], *text, *report = NULL;
    size_t len, reportlen = 0;
    FILE *out;
    long bad;

    if (argc - opt != 5) {
        fprintf(stderr, "usage: checkdata [-v|-q] TABLE LISTING REFERENCE CBUILD\n");
        return 2;
    }
    argv += opt;
    if (!(text = read_all(argv[1], &len))) {
        fprintf(stderr, "checkdata: cannot read %s\n", argv[1]);
        return 1;
    }
    if (checkdata_table_parse(text, len, &table, err, sizeof err)) {
        fprintf(stderr, "checkdata: %s: %s\n", argv[1], err);
        return 1;
    }
    free(text);
    if (listing_read(argv[2], &listing, err, sizeof err)) {
        fprintf(stderr, "checkdata: %s: %s\n", argv[2], err);
        return 1;
    }
    if (load_read(argv[3], &ref, err, sizeof err) || ref.nhunks < 1) {
        fprintf(stderr, "checkdata: %s: %s\n", argv[3], ref.nhunks ? err : "no hunk");
        return 1;
    }
    if (load_read(argv[4], &c, err, sizeof err)) {
        fprintf(stderr, "checkdata: %s: %s\n", argv[4], err);
        return 1;
    }
    out = quiet ? open_memstream(&report, &reportlen) : stdout;
    if (!out)
        return 1;
    bad = checkdata_run(&table, &listing, &ref.hunks[0], &c, verbose, out);
    if (quiet) {
        fclose(out);
        if (bad)
            fwrite(report, 1, reportlen, stdout);
        free(report);
    }
    load_free(&c);
    load_free(&ref);
    listing_free(&listing);
    checkdata_table_free(&table);
    return bad != 0;
}
