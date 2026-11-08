/*
 * blitzlib join, extract, numbers - join Blitz Basic 2 library files into
 * one BigLib file (blitz2:acidlibs, blitz2:deflibs; format:
 * lib/biglib/biglib.h), take one out of it, check the library numbers.
 *
 *   blitzlib join OUT LIB.obj...
 *   blitzlib extract BIGLIB NUMBER|LIB.obj OUT
 *   blitzlib numbers FILE...
 *
 * join: every input must be a well-formed AmigaDOS load file (lib/hunk).
 * extract: the library with that number (or the number of LIB.obj, e.g.
 * the original of a library built here) as a file of its own.
 * numbers: no two libraries of the FILEs (BigLib or library files, e.g.
 * acidlibs, deflibs and the userlibs of a variant) with the same number;
 * exit status 1 with both named if there are.
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "biglib.h"
#include "hunk.h"
#include "cmds.h"

static const char *me; /* "blitzlib SUB", the prefix of the messages */

static int join(const char *out, int n, char **files)
{
    struct loadfile *lf = calloc(n ? (size_t)n : 1, sizeof *lf);
    struct biglib_entry *e = calloc(n ? (size_t)n : 1, sizeof *e);
    FILE *f;
    int i, rc = 1;

    if (!lf || !e) {
        fprintf(stderr, "%s: out of memory\n", me);
        goto done;
    }
    for (i = 0; i < n; i++) {
        char err[200];

        if (load_read(files[i], &lf[i], err, sizeof err)) {
            fprintf(stderr, "%s: %s: %s\n", me, files[i], err);
            goto done;
        }
        e[i].data = lf[i].data;
        e[i].size = (uint32_t)lf[i].size;
    }
    f = fopen(out, "wb");
    if (!f) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        goto done;
    }
    if (biglib_write(f, e, (size_t)n)) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        fclose(f);
        remove(out);
        goto done;
    }
    if (fclose(f)) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        remove(out);
        goto done;
    }
    printf("%s: %d libs\n", out, n);
    rc = 0;
done:
    for (i = 0; lf && i < n; i++)
        load_free(&lf[i]);
    free(lf);
    free(e);
    return rc;
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

/* the library WHICH (a number, or a library file: its number) of BIGLIB */
static int extract(const char *big, const char *which, const char *out)
{
    size_t size, n, len;
    uint8_t *buf = read_all(big, &size), *lib = NULL;
    struct biglib_entry *e = NULL;
    char err[200], *end;
    unsigned num = (unsigned)strtoul(which, &end, 0);
    long i;
    int rc = 1;
    FILE *f;

    if (!buf || biglib_read(buf, size, &e, &n, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, big, buf ? err : "cannot read");
        goto done;
    }
    if (!*which || *end) {              /* not a number: a library file */
        if (!(lib = read_all(which, &len)) || biglib_number(lib, len, &num)) {
            fprintf(stderr, "%s: %s: no library file\n", me, which);
            goto done;
        }
    }
    if ((i = biglib_find(e, n, num)) < 0) {
        fprintf(stderr, "%s: %s: no library %u\n", me, big, num & 0xFFFF);
        goto done;
    }
    if (!(f = fopen(out, "wb")) || fwrite(e[i].data, 1, e[i].size, f) != e[i].size) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        if (f)
            fclose(f);
        goto done;
    }
    if (fclose(f)) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        goto done;
    }
    rc = 0;
done:
    free(e);
    free(buf);
    free(lib);
    return rc;
}

/* room for n entries in the three arrays of check; 0 = out of memory */
static int grow(struct biglib_entry **all, int **from, int **index, size_t n)
{
    struct biglib_entry *a = realloc(*all, (n ? n : 1) * sizeof *a);
    int *f, *x;

    if (a)
        *all = a;
    f = realloc(*from, (n ? n : 1) * sizeof *f);
    if (f)
        *from = f;
    x = realloc(*index, (n ? n : 1) * sizeof *x);
    if (x)
        *index = x;
    return a && f && x;
}

/* the libraries of FILES (BigLib or library files) with the same number */
static int check(int n, char **files)
{
    struct biglib_entry *all = NULL, *e;
    size_t nall = 0, ne, k, earlier;
    uint8_t **bufs = calloc(n ? (size_t)n : 1, sizeof *bufs);
    int *from = NULL, *index = NULL, i, rc = 1;
    long c;

    if (!bufs)
        goto nomem;
    for (i = 0; i < n; i++) {
        char err[200];
        size_t size;

        if (!(bufs[i] = read_all(files[i], &size))) {
            fprintf(stderr, "%s: %s: cannot read\n", me, files[i]);
            goto done;
        }
        if (size >= 4 && be32(bufs[i]) == HUNK_HEADER) {      /* one library */
            if (!(e = malloc(sizeof *e)))
                goto nomem;
            e->data = bufs[i];
            e->size = (uint32_t)size;
            ne = 1;
        } else if (biglib_read(bufs[i], size, &e, &ne, err, sizeof err)) {
            fprintf(stderr, "%s: %s: %s\n", me, files[i], err);
            goto done;
        }
        if (!grow(&all, &from, &index, nall + ne)) {
            free(e);
            goto nomem;
        }
        for (k = 0; k < ne; k++) {
            all[nall] = e[k];
            from[nall] = i;
            index[nall++] = (int)k + 1;
        }
        free(e);
    }
    if ((c = biglib_clash(all, nall, &earlier)) >= 0) {
        unsigned num = 0;

        biglib_number(all[c].data, all[c].size, &num);
        fprintf(stderr, "%s: library number %u twice: %s (library %d) and %s (library %d)\n", me,
                num, files[from[earlier]], index[earlier], files[from[c]], index[c]);
        goto done;
    }
    rc = 0;
    goto done;
nomem:
    fprintf(stderr, "%s: out of memory\n", me);
done:
    for (i = 0; bufs && i < n; i++)
        free(bufs[i]);
    free(bufs);
    free(all);
    free(from);
    free(index);
    return rc;
}

const char cmd_join_usage[] =
    "usage: blitzlib join OUT LIB.obj...\n"
    "  join Blitz Basic 2 library files into one BigLib file\n";
const char cmd_extract_usage[] =
    "usage: blitzlib extract BIGLIB NUMBER|LIB.obj OUT\n"
    "  take the library with that number (or the number of LIB.obj) out of a\n"
    "  BigLib file\n";
const char cmd_numbers_usage[] =
    "usage: blitzlib numbers FILE...\n"
    "  no library number twice in the FILEs (BigLib or library files)\n";

int cmd_join(int argc, char **argv)
{
    me = argv[0];
    if (argc < 2) {
        fputs(cmd_join_usage, stderr);
        return 2;
    }
    return join(argv[1], argc - 2, argv + 2);
}

int cmd_extract(int argc, char **argv)
{
    me = argv[0];
    if (argc != 4) {
        fputs(cmd_extract_usage, stderr);
        return 2;
    }
    return extract(argv[1], argv[2], argv[3]);
}

int cmd_numbers(int argc, char **argv)
{
    me = argv[0];
    if (argc < 2) {
        fputs(cmd_numbers_usage, stderr);
        return 2;
    }
    return check(argc - 1, argv + 1);
}
