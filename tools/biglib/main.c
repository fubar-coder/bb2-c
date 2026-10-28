/*
 * biglib - join Blitz Basic 2 library files into one BigLib file
 * (blitz2:acidlibs, blitz2:deflibs; format: lib/biglib/biglib.h), or take
 * one out of it.
 *
 *   biglib join OUT LIB.obj...
 *   biglib extract BIGLIB NUMBER|LIB.obj OUT
 *
 * join: every input must be a well-formed AmigaDOS load file (lib/hunk).
 * extract: the library with that number (or the number of LIB.obj, e.g.
 * the original of a library built here) as a file of its own.
 */
#include <stdlib.h>
#include <string.h>
#include "biglib.h"
#include "hunk.h"

static int join(const char *out, int n, char **files)
{
    struct loadfile *lf = calloc(n ? (size_t)n : 1, sizeof *lf);
    struct biglib_entry *e = calloc(n ? (size_t)n : 1, sizeof *e);
    FILE *f;
    int i, rc = 1;

    if (!lf || !e) {
        fprintf(stderr, "biglib: out of memory\n");
        goto done;
    }
    for (i = 0; i < n; i++) {
        char err[200];

        if (load_read(files[i], &lf[i], err, sizeof err)) {
            fprintf(stderr, "biglib: %s: %s\n", files[i], err);
            goto done;
        }
        e[i].data = lf[i].data;
        e[i].size = (uint32_t)lf[i].size;
    }
    f = fopen(out, "wb");
    if (!f) {
        perror(out);
        goto done;
    }
    if (biglib_write(f, e, (size_t)n)) {
        perror(out);
        fclose(f);
        remove(out);
        goto done;
    }
    if (fclose(f)) {
        perror(out);
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
        fprintf(stderr, "biglib: %s: %s\n", big, buf ? err : "cannot read");
        goto done;
    }
    if (!*which || *end) {              /* not a number: a library file */
        if (!(lib = read_all(which, &len)) || biglib_number(lib, len, &num)) {
            fprintf(stderr, "biglib: %s: no library file\n", which);
            goto done;
        }
    }
    if ((i = biglib_find(e, n, num)) < 0) {
        fprintf(stderr, "biglib: %s: no library %u\n", big, num & 0xFFFF);
        goto done;
    }
    if (!(f = fopen(out, "wb")) || fwrite(e[i].data, 1, e[i].size, f) != e[i].size) {
        perror(out);
        if (f)
            fclose(f);
        goto done;
    }
    if (fclose(f)) {
        perror(out);
        goto done;
    }
    rc = 0;
done:
    free(e);
    free(buf);
    free(lib);
    return rc;
}

int main(int argc, char **argv)
{
    if (argc >= 3 && !strcmp(argv[1], "join"))
        return join(argv[2], argc - 3, argv + 3);
    if (argc == 5 && !strcmp(argv[1], "extract"))
        return extract(argv[2], argv[3], argv[4]);
    fprintf(stderr, "usage: biglib join OUT LIB.obj...\n"
                    "       biglib extract BIGLIB NUMBER|LIB.obj OUT\n");
    return 2;
}
