/*
 * bbdetok - a tokenised Blitz 2 / AmiBlitz source as plain text, line N
 * of the text = line N of the file (lib/bbdetok/bbdetok.h), for reading
 * and diffing sources written with other editors and libraries.
 *
 *   bbdetok [--ted] [--strict] [--unknown] TOKENS... IN [OUT]
 *
 * TOKENS, in the compiler's order (the first entry of a number wins):
 *   --tokens FILE   a token list file ("XXXX name" per line: tools/
 *                   blitztokens, tests/data/ted-tokens.txt)
 *   --blitz2 FILE   a compiler's own tokens (blitz2, an AmiBlitz compiler)
 *   --last XXXX     for the next --blitz2: of its several lists (blitz2-ab3)
 *                   the one whose last token is $XXXX
 *   --lib FILE      a library or BigLib file (acidlibs, deflibs, userlibs)
 * --ted: TED's rules (every byte from $80 a token, also in strings and
 * comments), else AmiBlitz's (PED, AmiBlitz 2/3: strings and comments as
 * they are).  A token not in the list is written as {L:N}; the count goes
 * to stderr, --unknown lists every such number with its count, --strict
 * makes it an error.  Without OUT the text goes to stdout.
 *
 * Exit status 1 if a file cannot be read, a token list is refused, the
 * source ends with a token byte or (--strict) has unknown tokens, or OUT
 * cannot be written (OUT is then removed); 2: wrong arguments.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bbdetok.h"

static const char usage[] =
    "usage: bbdetok [--ted] [--strict] [--unknown] TOKENS... IN [OUT]\n"
    "A tokenised Blitz 2 / AmiBlitz source as plain text, line for line.\n"
    "TOKENS (in the compiler's order, the first entry of a number wins):\n"
    "  --tokens FILE   a token list file (\"XXXX name\" per line, blitztokens)\n"
    "  --blitz2 FILE   a compiler's own tokens\n"
    "  --last XXXX     for the next --blitz2: its list that ends with $XXXX\n"
    "  --lib FILE      a library or BigLib file (acidlibs, deflibs, userlibs)\n"
    "--ted      TED's rules: every byte from $80 a token, also in strings and\n"
    "           comments (default: AmiBlitz's, strings and comments as they are)\n"
    "--strict   fail on a token the list does not have (else written as {L:N})\n"
    "--unknown  list the unknown tokens on stderr: {L:N} $XXXX count\n"
    "Without OUT the text goes to stdout.\n";

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
    static struct bbdetok_result r;
    struct blitztokens list = { 0 };
    struct bbdetok d = { 0 };
    int rules = BBDETOK_AB, strict = 0, unknown = 0, ntok = 0, rc = 1, i, a;
    const char *in, *outpath = NULL;
    uint8_t **bufs, *src = NULL;
    unsigned long last = 0;
    char err[300], *text = NULL;
    size_t len, textlen;
    FILE *f;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    bufs = calloc((size_t)argc, sizeof *bufs);
    if (!bufs) {
        fprintf(stderr, "bbdetok: out of memory\n");
        return 1;
    }
    for (a = 1; a < argc && argv[a][0] == '-' && argv[a][1] == '-'; a++) {
        const char *o = argv[a];
        int r2;

        if (!strcmp(o, "--ted"))
            rules = BBDETOK_TED;
        else if (!strcmp(o, "--strict"))
            strict = 1;
        else if (!strcmp(o, "--unknown"))
            unknown = 1;
        else if (a + 1 < argc && !strcmp(o, "--last")) {
            char *end;

            last = strtoul(argv[++a], &end, 16);
            if (*end || !*argv[a] || !last || last > 0xFFFF) {
                fprintf(stderr, "bbdetok: --last: not a token number: %s\n", argv[a]);
                rc = 2;
                goto done;
            }
        } else if (a + 1 < argc
                   && (!strcmp(o, "--tokens") || !strcmp(o, "--blitz2") || !strcmp(o, "--lib"))) {
            const char *path = argv[++a];

            if (!(bufs[a] = read_all(path, &len))) {
                fprintf(stderr, "bbdetok: cannot read %s\n", path);
                goto done;
            }
            if (o[2] == 't')
                r2 = blitztokens_read((char *)bufs[a], len, &list, err, sizeof err);
            else if (o[2] == 'b') {
                r2 = blitztokens_compiler_last(bufs[a], len, (uint16_t)last, &list, err,
                                               sizeof err);
                last = 0;
            } else
                r2 = blitztokens_libraries(bufs[a], len, &list, err, sizeof err);
            if (r2) {
                fprintf(stderr, "bbdetok: %s: %s\n", path, err);
                goto done;
            }
            ntok++;
        } else {
            fprintf(stderr, "bbdetok: unknown option or missing argument: %s\n", o);
            fputs(usage, stderr);
            rc = 2;
            goto done;
        }
    }
    if (!ntok || last || argc - a < 1 || argc - a > 2) {
        fputs(usage, stderr);
        rc = 2;
        goto done;
    }
    in = argv[a];
    outpath = argc - a == 2 ? argv[a + 1] : NULL;
    if (bbdetok_init(&d, &list)) {
        fprintf(stderr, "bbdetok: out of memory\n");
        goto done;
    }
    if (!(src = read_all(in, &len))) {
        fprintf(stderr, "bbdetok: cannot read %s\n", in);
        goto done;
    }
    if (bbdetok_text(&d, rules, src, len, &text, &textlen, &r, err, sizeof err)) {
        fprintf(stderr, "bbdetok: %s: %s\n", in, err);
        goto done;
    }
    if (r.open)
        fprintf(stderr, "bbdetok: %s: note: the last line has no 0 byte (a text file?)\n", in);
    if (r.unknown)
        fprintf(stderr, "bbdetok: %s: %zu of %zu tokens unknown (%zu different numbers)\n", in,
                r.unknown, r.tokens, r.kinds);
    if (unknown)
        for (i = 0; i < 32768; i++)
            if (r.count[i])
                fprintf(stderr, "{%d:%d} $%04X %u\n", i >> 7, i & 0x7F, i | 0x8000,
                        (unsigned)r.count[i]);
    if (strict && r.unknown)
        goto done;
    f = outpath ? fopen(outpath, "wb") : stdout;
    if (!f) {
        fprintf(stderr, "bbdetok: cannot write %s\n", outpath);
        goto done;
    }
    i = fwrite(text, 1, textlen, f) != textlen;
    if ((outpath ? fclose(f) : fflush(f)) || i) {
        fprintf(stderr, "bbdetok: cannot write %s\n", outpath ? outpath : "stdout");
        goto done;
    }
    rc = 0;
done:
    if (rc && outpath)
        remove(outpath);                /* no OUT of an earlier run stays */
    for (i = 0; i < argc; i++)
        free(bufs[i]);
    free(bufs);
    free(src);
    free(text);
    bbdetok_free(&d);
    blitztokens_free(&list);
    return rc;
}
