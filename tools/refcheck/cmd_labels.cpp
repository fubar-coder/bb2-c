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
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "checklabels.h"
}
#include "refcheck.h"

static char **paths;
static size_t npaths;

static int collect(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    size_t n = strlen(path);

    (void)st;
    (void)ftw;
    if (type == FTW_F && n > 2 && !strcmp(path + n - 2, ".c")) {
        char **p = (char **)realloc(paths, (npaths + 1) * sizeof *p);

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
        && (s = (char *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
        s[n] = 0;
        *len = (size_t)n;
    } else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

static const char *me; /* "refcheck labels", the prefix of the messages */

static int run(bool verbose, bool quiet, const char *asm_path, const char *srcdir)
{
    char *report = NULL;
    size_t reportlen = 0;
    FILE *out;
    struct code_labels labels;
    struct label_source *src;
    const char *prefix;
    char *text;
    size_t len, i;
    long bad;

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
    if (nftw(srcdir, collect, 16, FTW_PHYS)) {
        fprintf(stderr, "%s: cannot read %s\n", me, srcdir);
        return 1;
    }
    qsort(paths, npaths, sizeof *paths, by_name);
    if (!(src = (struct label_source *)calloc(npaths ? npaths : 1, sizeof *src)))
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

void cmd_labels(CLI::App &app, int &status)
{
    struct options {
        bool verbose = false, quiet = false;
        std::string asm_path, srcdir;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("labels", "every code label of FILE.s exactly once in the C sources of SRCDIR");
    CLI::Option *v = sub->add_flag("-v", o->verbose, "every label");
    sub->add_flag("-q", o->quiet, "the report only if a label is missing or doubled")->excludes(v);
    sub->add_option("FILE.s", o->asm_path, "the assembler source (FILE in \"FILE:NNN label\" is its name)")
        ->required();
    sub->add_option("SRCDIR", o->srcdir, "the directory of the C sources (every *.c below it)")->required();
    sub->footer("Prints the missing and doubled labels and a summary. Exit status 0, 1 a label is "
                "missing or doubled or an input cannot be read, 2 usage error.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = run(o->verbose, o->quiet, o->asm_path.c_str(), o->srcdir.c_str());
    });
}
