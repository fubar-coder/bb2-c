/*
 * bbsrc comment - an edit of the lines of a tokenised Blitz Basic 2 source
 * on its bytes (lib/bblines/bblines.h).
 *
 *   bbsrc comment TOKEN IN OUT
 *
 * Every line whose statement is the token TOKEN (hex, e.g. 80BF = INCDIR)
 * becomes ";"; the other lines stay byte for byte.  The reference build of
 * the RWE sources (libs/userlibs/rwe/refbuild.cmake) neutralises the
 * INCDIR lines with it.  Exit status 1 on an error (OUT is not written),
 * 2 on wrong arguments.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bblines.h"
#include "bbsrc.h"

const char cmd_comment_usage[] =
    "usage: bbsrc comment TOKEN IN OUT\n"
    "Every line of the tokenised Blitz 2 source IN whose statement is the\n"
    "token TOKEN (hex, e.g. 80BF = INCDIR) becomes \";\" in OUT, the other\n"
    "lines stay.\n";

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

int cmd_comment(int argc, char **argv)
{
    const char *me = argv[0];
    unsigned long token;
    uint8_t *src, *out;
    size_t len, outlen, count;
    char err[300], *e;
    int a = 1;
    FILE *f;

    if (argc != 4) {
        fputs(cmd_comment_usage, stderr);
        return 2;
    }
    token = strtoul(argv[a], &e, 16);
    if (!*argv[a] || *e || token > 0xFFFF) {
        fprintf(stderr, "%s: %s is not a token word (hex)\n", me, argv[a]);
        return 2;
    }
    a++;
    if (!(src = read_all(argv[a], &len))) {
        fprintf(stderr, "%s: %s: %s\n", me, argv[a], strerror(errno));
        return 1;
    }
    if (bblines_comment(src, len, (uint16_t)token, &out, &outlen, &count, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, argv[a], err);
        free(src);
        return 1;
    }
    free(src);
    if (!(f = fopen(argv[a + 1], "wb")) || fwrite(out, 1, outlen, f) != outlen || fclose(f)) {
        fprintf(stderr, "%s: %s: %s\n", me, argv[a + 1], strerror(errno));
        remove(argv[a + 1]);
        free(out);
        return 1;
    }
    free(out);
    return 0;
}
