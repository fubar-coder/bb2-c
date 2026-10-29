/*
 * blitzlibcompare - the declarations of every library of a BigLib file
 * against the original (blitzlib_compare, see blitzlib.h).
 *
 *   blitzlibcompare ORIGINAL NEW          e.g. CD BLITZ2/DEFLIBS and deflibs
 *
 * Both must hold the same libraries in the same order (the order of the
 * compiler's token list).  Prints the differences and a summary; exit
 * status 1 if there are any, 77 (skipped) if ORIGINAL does not exist (the
 * CD is not in the repository: fetch-originals).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "biglib.h"
#include "blitzlib.h"

struct biglibfile {
    uint8_t *buf;
    struct biglib_entry *e;
    size_t n;
    struct loadfile *lf;
};

static int read_biglib(const char *path, struct biglibfile *b)
{
    FILE *f = fopen(path, "rb");
    char err[200];
    long size;
    size_t i;

    b->buf = NULL;
    b->e = NULL;
    b->lf = NULL;
    if (!f || fseek(f, 0, SEEK_END) || (size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)
        || !(b->buf = malloc((size_t)size + 1)) || fread(b->buf, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "blitzlibcompare: cannot read %s\n", path);
        if (f)
            fclose(f);
        return -1;
    }
    fclose(f);
    if (biglib_read(b->buf, (size_t)size, &b->e, &b->n, err, sizeof err)) {
        fprintf(stderr, "blitzlibcompare: %s: %s\n", path, err);
        return -1;
    }
    if (!(b->lf = calloc(b->n ? b->n : 1, sizeof *b->lf)))
        return -1;
    for (i = 0; i < b->n; i++)
        if (load_parse(b->e[i].data, b->e[i].size, &b->lf[i], err, sizeof err)
            || b->lf[i].nhunks < 1 || b->lf[i].hunks[0].size < 6) {
            fprintf(stderr, "blitzlibcompare: %s: library %zu: %s\n", path, i + 1,
                    b->lf[i].nhunks < 1 ? "no hunk" : err);
            return -1;
        }
    return 0;
}

static unsigned libnum(const struct loadfile *lf)
{
    return be16(lf->hunks[0].data + 4);
}

static void free_biglib(struct biglibfile *b)
{
    size_t i;

    for (i = 0; b->lf && i < b->n; i++)
        load_free(&b->lf[i]);
    free(b->lf);
    free(b->e);
    free(b->buf);
}

static const char usage[] =
    "usage: blitzlibcompare ORIGINAL NEW\n"
    "  the declarations of every library of a BigLib file against the original\n";

int main(int argc, char **argv)
{
    struct biglibfile a, b;
    size_t i, n;
    long diffs = 0;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    if (argc != 3) {
        fputs(usage, stderr);
        return 2;
    }
    if (access(argv[1], F_OK)) {
        printf("blitzlibcompare: no %s - skipped (fetch-originals)\n", argv[1]);
        return 77;
    }
    if (read_biglib(argv[1], &a) || read_biglib(argv[2], &b))
        return 1;
    if (a.n != b.n) {
        printf("%s: %zu libraries, %s: %zu\n", argv[1], a.n, argv[2], b.n);
        diffs++;
    }
    n = a.n < b.n ? a.n : b.n;
    for (i = 0; i < n; i++) {
        char name[32];
        int d;

        if (libnum(&a.lf[i]) != libnum(&b.lf[i])) {
            printf("library %zu: number %u != %u (order of the libraries)\n", i + 1,
                   libnum(&a.lf[i]), libnum(&b.lf[i]));
            diffs++;
            continue;
        }
        snprintf(name, sizeof name, "lib %u", libnum(&a.lf[i]));
        d = blitzlib_compare(&a.lf[i].hunks[0], &b.lf[i].hunks[0], name, stdout);
        diffs += d < 0 ? 1 : d;
    }
    printf("blitzlibcompare: %zu libraries, %ld differences in the declarations\n", n, diffs);
    free_biglib(&a);
    free_biglib(&b);
    return diffs != 0;
}
