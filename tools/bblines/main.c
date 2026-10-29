/*
 * bblines - edits of the lines of a tokenised Blitz Basic 2 source on its
 * bytes (lib/bblines/bblines.h).
 *
 *   bblines --comment TOKEN IN OUT
 *
 * --comment: every line whose statement is the token TOKEN (hex, e.g.
 * 80BF = INCDIR) becomes ";"; the other lines stay byte for byte.  The
 * reference build of the RWE sources (libs/userlibs/rwe/refbuild.cmake) neutralises the
 * INCDIR lines with it.  Exit status 1 on an error (OUT is not written),
 * 2 on wrong arguments.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bblines.h"

static void usage(FILE *f)
{
    fprintf(f, "usage: bblines --comment TOKEN IN OUT\n"
               "Every line of the tokenised Blitz 2 source IN whose statement is the\n"
               "token TOKEN (hex, e.g. 80BF = INCDIR) becomes \";\" in OUT, the other\n"
               "lines stay.\n");
}

static uint8_t *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
        *len = (size_t)n;
    else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

int main(int argc, char **argv)
{
    unsigned long token;
    uint8_t *src, *out;
    size_t len, outlen, count;
    char err[300], *e;
    int a = 1;
    FILE *f;

    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        usage(stdout);
        return 0;
    }
    if (argc != 5 || strcmp(argv[a], "--comment")) {
        usage(stderr);
        return 2;
    }
    token = strtoul(argv[a + 1], &e, 16);
    if (!*argv[a + 1] || *e || token > 0xFFFF) {
        fprintf(stderr, "bblines: %s is not a token word (hex)\n", argv[a + 1]);
        return 2;
    }
    a += 2;
    if (!(src = read_all(argv[a], &len))) {
        perror(argv[a]);
        return 1;
    }
    if (bblines_comment(src, len, (uint16_t)token, &out, &outlen, &count, err, sizeof err)) {
        fprintf(stderr, "bblines: %s: %s\n", argv[a], err);
        free(src);
        return 1;
    }
    free(src);
    if (!(f = fopen(argv[a + 1], "wb")) || fwrite(out, 1, outlen, f) != outlen || fclose(f)) {
        perror(argv[a + 1]);
        remove(argv[a + 1]);
        free(out);
        return 1;
    }
    free(out);
    return 0;
}
