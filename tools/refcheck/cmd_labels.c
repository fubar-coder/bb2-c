/*
 * refcheck labels - every code label of an assembler source appears exactly
 * once as "FILE:NNN label" in the C sources of a directory (see
 * checklabels.h).
 *
 *   refcheck labels [-v|-q] ted.s SRCDIR    (FILE = the name of ted.s)
 *
 * Reads every *.c under SRCDIR.  Prints the missing / doubled labels and a
 * summary (-q: only if there are any, -v: every label); exit status 1 if
 * any.
 */
#define _XOPEN_SOURCE 700       /* nftw, strdup */
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "checklabels.h"
#include "refcheck.h"

static char **paths;
static size_t npaths;

static int collect(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    size_t n = strlen(path);

    (void)st;
    (void)ftw;
    if (type == FTW_F && n > 2 && !strcmp(path + n - 2, ".c")) {
        char **p = realloc(paths, (npaths + 1) * sizeof *p);

        if (!p || !(p[npaths] = strdup(path)))
            return 1;
        paths = p;
        npaths++;
    }
    return 0;
}

static int by_name(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

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

const char cmd_labels_usage[] =
    "usage: refcheck labels [-v|-q] FILE.s SRCDIR\n"
    "  every code label of FILE.s exactly once in the C sources of SRCDIR\n";

static const char *me; /* "refcheck labels", the prefix of the messages */

int cmd_labels(int argc, char **argv)
{
    int verbose = argc > 1 && !strcmp(argv[1], "-v");
    int quiet = argc > 1 && !strcmp(argv[1], "-q");
    int opt = verbose || quiet;
    char *report = NULL;
    size_t reportlen = 0;
    FILE *out;
    struct code_labels labels;
    struct label_source *src;
    const char *asm_path, *prefix;
    char *text;
    size_t len, i;
    long bad;

    me = argv[0];
    if (argc - opt != 3) {
        fputs(cmd_labels_usage, stderr);
        return 2;
    }
    asm_path = argv[1 + opt];
    prefix = strrchr(asm_path, '/') ? strrchr(asm_path, '/') + 1 : asm_path;
    if (!(text = read_all(asm_path, &len))) {
        fprintf(stderr, "%s: cannot read %s\n", me, asm_path);
        return 1;
    }
    if (code_labels_parse(text, len, &labels)) {
        fprintf(stderr, "%s: out of memory\n", me);
        return 1;
    }
    free(text);
    if (nftw(argv[2 + opt], collect, 16, FTW_PHYS)) {
        fprintf(stderr, "%s: cannot read %s\n", me, argv[2 + opt]);
        return 1;
    }
    qsort(paths, npaths, sizeof *paths, by_name);
    if (!(src = calloc(npaths ? npaths : 1, sizeof *src)))
        return 1;
    for (i = 0; i < npaths; i++) {
        src[i].name = strrchr(paths[i], '/') ? strrchr(paths[i], '/') + 1 : paths[i];
        if (!(src[i].text = read_all(paths[i], &src[i].len))) {
            fprintf(stderr, "%s: cannot read %s\n", me, paths[i]);
            return 1;
        }
    }
    out = quiet ? open_memstream(&report, &reportlen) : stdout;
    if (!out)
        return 1;
    bad = labels_check(&labels, prefix, src, npaths, verbose, out);
    if (quiet) {
        fclose(out);
        if (bad)
            fwrite(report, 1, reportlen, stdout);
        free(report);
    }
    for (i = 0; i < npaths; i++) {
        free((char *)src[i].text);
        free(paths[i]);
    }
    free(src);
    free(paths);
    code_labels_free(&labels);
    return bad != 0;
}
