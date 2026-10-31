/*
 * blitztokens - the token list of Blitz Basic 2 as the compiler hands it
 * to the editor, from the files: the compiler and its libraries
 * (format: lib/blitztokens/blitztokens.h, as tests/data/ted-tokens.txt).
 *
 *   blitztokens [-o OUT] [-last XXXX] BLITZ2 [LIBS...]
 *
 * BLITZ2: the compiler (blitz2:blitz2); LIBS: library files or BigLib
 * files in the order the compiler links them - blitz2:acidlibs,
 * blitz2:deflibs, then the files of blitz2:userlibs.  Without -o the list
 * goes to stdout.  -last: the compiler has several lists (blitz2-ab3),
 * take the one whose last token is $XXXX (hex; AmiBlitz3's: 80DB).
 */
#include <stdlib.h>
#include <string.h>
#include "blitztokens.h"

static void usage(FILE *f)
{
    fprintf(f, "usage: blitztokens [-o OUT] [-last XXXX] BLITZ2 [LIBS...]\n"
            "The token list of Blitz Basic 2 as the compiler hands it to the editor:\n"
            "the compiler's own tokens, then the tokens of LIBS (library or BigLib\n"
            "files, in the compiler's order: acidlibs, deflibs, userlibs), one\n"
            "\"XXXX name\" per line like tests/data/ted-tokens.txt.\n"
            "-last XXXX: of several lists in BLITZ2 (blitz2-ab3) the one whose last\n"
            "token is $XXXX (hex).\n");
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
    struct blitztokens list = { 0 };
    const char *out = NULL;
    uint8_t **bufs;
    int i, first = 1, rc = 1;
    unsigned long last = 0;
    FILE *f;

    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        usage(stdout);
        return 0;
    }
    for (;;) {
        if (argc > first + 1 && !strcmp(argv[first], "-o")) {
            out = argv[first + 1];
            first += 2;
        } else if (argc > first + 1 && !strcmp(argv[first], "-last")) {
            char *end;

            last = strtoul(argv[first + 1], &end, 16);
            if (*end || !last || last > 0xFFFF) {
                fprintf(stderr, "blitztokens: -last: not a token number: %s\n", argv[first + 1]);
                return 2;
            }
            first += 2;
        } else
            break;
    }
    if (first >= argc) {
        usage(stderr);
        return 2;
    }
    bufs = calloc((size_t)argc, sizeof *bufs);
    if (!bufs) {
        fprintf(stderr, "blitztokens: out of memory\n");
        return 1;
    }
    for (i = first; i < argc; i++) {
        char err[300];
        size_t len;
        int r;

        if (!(bufs[i] = read_all(argv[i], &len))) {
            perror(argv[i]);
            goto done;
        }
        r = i == first ? blitztokens_compiler_last(bufs[i], len, (uint16_t)last, &list,
                                                   err, sizeof err)
                       : blitztokens_libraries(bufs[i], len, &list, err, sizeof err);
        if (r) {
            fprintf(stderr, "blitztokens: %s: %s\n", argv[i], err);
            goto done;
        }
    }
    f = out ? fopen(out, "w") : stdout;
    if (!f) {
        perror(out);
        goto done;
    }
    if (blitztokens_write(f, &list) || (out ? fclose(f) : fflush(f))) {
        perror(out ? out : "stdout");
        if (out)
            remove(out);
        goto done;
    }
    rc = 0;
done:
    for (i = first; i < argc; i++)
        free(bufs[i]);
    free(bufs);
    blitztokens_free(&list);
    return rc;
}
