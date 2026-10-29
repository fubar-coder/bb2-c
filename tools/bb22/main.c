/*
 * bb22 - a Blitz 2 source in the "BB22" format (tools/lib/bb22/bb22.h,
 * docs/rwelibs/README.md 4.1) in TED's format.
 *
 *   bb22 IN OUT       convert: OUT = the lines of IN, each ended by 0
 *   bb22 --info IN    version, header words, line count, longest line
 *
 * Exit status 1 if IN cannot be read or is refused (message with the
 * offset or line; OUT is then not written) or OUT cannot be written
 * (OUT is removed); 2: wrong arguments.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bb22.h"

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *d = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (d = malloc((size_t)n + 1)) && fread(d, 1, (size_t)n, f) == (size_t)n)
        *len = (size_t)n;
    else {
        free(d);
        d = NULL;
    }
    fclose(f);
    return d;
}

static const char usage[] =
    "usage: bb22 IN OUT\n"
    "       bb22 --info IN\n"
    "A Blitz 2 source in the BB22 format (some of RWE's library sources,\n"
    "1997) in TED's format (lines of tokenised text, each ended by 0);\n"
    "--info shows the header (version, cursor), the line count and the\n"
    "longest line. Refuses whatever it does not know.\n";

int main(int argc, char **argv)
{
    const char *in;
    struct bb22 b;
    char err[200];
    uint8_t *data;
    size_t len;
    bool info;
    FILE *f;
    int bad;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    info = argc == 3 && !strcmp(argv[1], "--info");
    if (argc != 3) {
        fputs(usage, stderr);
        return 2;
    }
    in = info ? argv[2] : argv[1];
    if (!(data = read_file(in, &len))) {
        fprintf(stderr, "bb22: cannot read %s\n", in);
        return 1;
    }
    if (bb22_parse(data, len, &b, err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", in, err);
        free(data);
        return 1;
    }
    if (info) {
        printf("version %u (header %zu bytes, line prefix %zu)\n", b.version, b.header_size,
               b.prefix_size);
        printf("cursor: column %u, row %u, top line %u (line %u, 0-based)\n", b.column, b.row,
               b.top, b.top + b.row);
        printf("lines %zu, longest %zu bytes\n", b.nlines, b.max_len);
        bb22_free(&b);
        free(data);
        return 0;
    }
    f = fopen(argv[2], "wb");
    bad = !f || bb22_write_ted(&b, data, f);
    if (f && fclose(f))
        bad = 1;
    bb22_free(&b);
    free(data);
    if (bad) {
        fprintf(stderr, "bb22: cannot write %s\n", argv[2]);
        remove(argv[2]);
        return 1;
    }
    return 0;
}
