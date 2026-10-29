/*
 * mktestdata - binary test data (8SVX, ILBM, MED and tracker modules) from
 * a text description (tools/lib/mktestdata/mktestdata.h).
 *
 *   mktestdata SPEC OUT
 *
 * Writes OUT only when the description is valid; exit status 1 on an
 * error (message with the line of SPEC).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mktestdata.h"

static char *read_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

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
    return s;
}

static const char usage[] =
    "usage: mktestdata SPEC OUT\n"
    "  binary test data (8SVX, ILBM, MED, tracker modules) from a text description\n";

int main(int argc, char **argv)
{
    char err[200], *spec;
    uint8_t *data;
    size_t len;
    FILE *f;
    int bad;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    if (argc != 3) {
        fputs(usage, stderr);
        return 2;
    }
    if (!(spec = read_text(argv[1]))) {
        fprintf(stderr, "mktestdata: cannot read %s\n", argv[1]);
        return 1;
    }
    if (mktestdata_build(spec, &data, &len, err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", argv[1], err);
        free(spec);
        return 1;
    }
    free(spec);
    f = fopen(argv[2], "wb");
    bad = !f || fwrite(data, 1, len, f) != len;
    if (f && fclose(f))
        bad = 1;
    free(data);
    if (bad) {
        fprintf(stderr, "mktestdata: cannot write %s\n", argv[2]);
        remove(argv[2]);
        return 1;
    }
    return 0;
}
