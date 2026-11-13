/*
 * blitzlib compare - the declarations of every library of a BigLib file
 * against the original (blitzlib_compare, see blitzlib.h).
 *
 *   blitzlib compare ORIGINAL NEW [INTENDED]
 *                              e.g. CD BLITZ2/DEFLIBS and deflibs
 *
 * ORIGINAL and NEW may also be single library files (a userlib against
 * its original).
 *
 * INTENDED: a text file of the intended differences (fixed original
 * bugs), one message line per line as printed, `#` comments; they are not
 * counted, and each of them must occur (a stale entry is a difference).
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
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "biglib.h"
#include "blitzlib.h"
}
#include "cmds.h"

static const char *me; /* "blitzlib compare", the prefix of the messages */

struct biglibfile {
    uint8_t *buf;
    struct biglib_entry *e;
    size_t n;
    struct loadfile *lf;
    struct blitzlib_decls decls;        /* declaration data: its libraries */
    const struct hunk **h;              /* the code hunk of every library */
};

/* declaration data (blitzlib decls) instead of a library file */
static int is_decls(const uint8_t *buf, long size)
{
    return (size >= 1 && buf[0] == '#') || (size >= 8 && !memcmp(buf, "library\t", 8));
}

static int read_biglib(const char *path, struct biglibfile *b)
{
    FILE *f = fopen(path, "rb");
    char err[200];
    long size;
    size_t i;

    b->buf = NULL;
    b->e = NULL;
    b->lf = NULL;
    b->h = NULL;
    b->n = 0;
    b->decls.n = 0;
    b->decls.hunks = NULL;
    if (!f || fseek(f, 0, SEEK_END) || (size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)
        || !(b->buf = (uint8_t *)malloc((size_t)size + 1)) || fread(b->buf, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "%s: cannot read %s\n", me, path);
        if (f)
            fclose(f);
        return -1;
    }
    fclose(f);
    if (is_decls(b->buf, size)) {
        if (blitzlib_decls_parse((const char *)b->buf, (size_t)size, &b->decls, err, sizeof err)) {
            fprintf(stderr, "%s: %s: %s\n", me, path, err);
            return -1;
        }
        b->n = b->decls.n;
        if (!(b->h = (const struct hunk **)calloc(b->n ? b->n : 1, sizeof *b->h)))
            return -1;
        for (i = 0; i < b->n; i++)
            b->h[i] = &b->decls.hunks[i];
        return 0;
    }
    if (size >= 4 && be32(b->buf) == HUNK_HEADER) {     /* one library file */
        if (!(b->e = (struct biglib_entry *)malloc(sizeof *b->e)))
            return -1;
        b->e->data = b->buf;
        b->e->size = (uint32_t)size;
        b->n = 1;
    } else if (biglib_read(b->buf, (size_t)size, &b->e, &b->n, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
        return -1;
    }
    if (!(b->lf = (struct loadfile *)calloc(b->n ? b->n : 1, sizeof *b->lf)))
        return -1;
    for (i = 0; i < b->n; i++)
        if (load_parse(b->e[i].data, b->e[i].size, &b->lf[i], err, sizeof err)
            || b->lf[i].nhunks < 1 || b->lf[i].hunks[0].size < 6) {
            fprintf(stderr, "%s: %s: library %zu: %s\n", me, path, i + 1,
                    b->lf[i].nhunks < 1 ? "no hunk" : err);
            return -1;
        }
    if (!(b->h = (const struct hunk **)calloc(b->n ? b->n : 1, sizeof *b->h)))
        return -1;
    for (i = 0; i < b->n; i++)
        b->h[i] = &b->lf[i].hunks[0];
    return 0;
}

static unsigned libnum(const struct hunk *h)
{
    return be16(h->data + 4);
}

static void free_biglib(struct biglibfile *b)
{
    size_t i;

    blitzlib_decls_free(&b->decls);
    free(b->h);
    for (i = 0; b->lf && i < b->n; i++)
        load_free(&b->lf[i]);
    free(b->lf);
    free(b->e);
    free(b->buf);
}

#define MAXINTENDED 100

/* the list of intended differences; the text stays allocated (the lines
   point into it) */
static long read_intended(const char *path, const char **lines, char **text)
{
    FILE *f = fopen(path, "rb");
    long size, n;

    *text = NULL;
    if (!f || fseek(f, 0, SEEK_END) || (size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)
        || !(*text = (char *)malloc((size_t)size + 1)) || fread(*text, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "%s: cannot read %s\n", me, path);
        if (f)
            fclose(f);
        return -1;
    }
    fclose(f);
    (*text)[size] = 0;
    n = blitzlib_intended_list(*text, lines, MAXINTENDED);
    if (n < 0)
        fprintf(stderr, "%s: %s: more than %d lines\n", me, path, MAXINTENDED);
    return n;
}

/* what was written to f, 0-terminated */
static char *read_back(FILE *f)
{
    long size = ftell(f);
    char *t;

    if (size < 0 || fseek(f, 0, SEEK_SET) || !(t = (char *)malloc((size_t)size + 1)))
        return NULL;
    if (fread(t, 1, (size_t)size, f) != (size_t)size) {
        free(t);
        return NULL;
    }
    t[size] = 0;
    return t;
}

static int compare(const char *original, const char *changed, const char *intended_path)
{
    struct biglibfile a, b;
    size_t i, n;
    long diffs = 0, nintended = 0;
    const char *intended[MAXINTENDED];
    unsigned char seen[MAXINTENDED] = { 0 };
    char *intendedtext = NULL;

    if (access(original, F_OK)) {
        printf("%s: no %s - skipped (fetch-originals)\n", me, original);
        return 77;
    }
    if (intended_path && (nintended = read_intended(intended_path, intended, &intendedtext)) < 0)
        return 1;
    if (read_biglib(original, &a) || read_biglib(changed, &b))
        return 1;
    if (a.n > b.n) {
        printf("%s: %zu libraries, %s: %zu\n", original, a.n, changed, b.n);
        diffs++;
    }
    n = a.n < b.n ? a.n : b.n;
    /* a library behind the original's: a difference of its own, which
       INTENDED can name (an addition, e.g. literallib, #1018) */
    for (i = n; i < b.n; i++) {
        char msg[96];

        snprintf(msg, sizeof msg, "lib %u: added behind the original's libraries\n",
                 libnum(b.h[i]));
        diffs += blitzlib_intended(msg, intended, (size_t)nintended, seen, stdout);
    }
    for (i = 0; i < n; i++) {
        char name[32], *msgs;
        FILE *f;
        int d;

        if (libnum(a.h[i]) != libnum(b.h[i])) {
            printf("library %zu: number %u != %u (order of the libraries)\n", i + 1,
                   libnum(a.h[i]), libnum(b.h[i]));
            diffs++;
            continue;
        }
        snprintf(name, sizeof name, "lib %u", libnum(a.h[i]));
        if (!(f = tmpfile())) {
            fprintf(stderr, "%s: no temporary file\n", me);
            return 1;
        }
        d = blitzlib_compare(a.h[i], b.h[i], name, f);
        msgs = read_back(f);
        fclose(f);
        if (!msgs) {
            fprintf(stderr, "%s: cannot read the messages back\n", me);
            return 1;
        }
        if (d < 0) {
            fputs(msgs, stdout);
            diffs++;
        } else
            diffs += blitzlib_intended(msgs, intended, (size_t)nintended, seen, stdout);
        free(msgs);
    }
    diffs += blitzlib_intended_stale(intended, (size_t)nintended, seen, stdout);
    printf("%s: %zu libraries, %ld differences in the declarations\n", me, n, diffs);
    free_biglib(&a);
    free_biglib(&b);
    free(intendedtext);
    return diffs != 0;
}

void cmd_compare(CLI::App &app, int &status)
{
    struct options {
        std::string original, changed, intended;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand(
        "compare", "the declarations of every library of a BigLib file (or of a library file) against the original");
    sub->add_option("ORIGINAL", o->original,
                    "the original BigLib or library file, or its declaration data (decls)")
        ->required();
    sub->add_option("NEW", o->changed, "the BigLib or library file built here")->required();
    sub->add_option("INTENDED", o->intended,
                    "the intended differences, one message line per line (# comments), each of them must occur");
    sub->footer("Both hold the same libraries in the same order. Prints the differences and a "
                "summary. Exit status 0 no difference, 1 a difference or a stale INTENDED line, or "
                "a file cannot be read, 2 usage error, 77 (skipped) ORIGINAL does not exist "
                "(fetch-originals).");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = compare(o->original.c_str(), o->changed.c_str(),
                         sub->count("INTENDED") ? o->intended.c_str() : NULL);
    });
}

/* blitzlib decls: the declaration data of every library of FILE */
static int decls(const char *path, const char *output)
{
    struct biglibfile b;
    std::string tmp = std::string(output) + ".new";
    const char *base = strrchr(path, '/');
    FILE *out;
    char err[200];
    size_t i;
    int rc = 0;

    if (read_biglib(path, &b))
        return 1;
    if (!(out = fopen(tmp.c_str(), "w"))) {
        fprintf(stderr, "%s: cannot write %s\n", me, tmp.c_str());
        free_biglib(&b);
        return 1;
    }
    fprintf(out, "# The declarations of %s (blitzlib decls): what blitzlib compare compares -\n"
                 "# header, token table, !libfin. Per entry: number, name, help text, the\n"
                 "# bytes before and behind the two texts (a nullsub: \"-\" and its bytes).\n"
                 "# Texts as they are (\\\\ a backslash, \\xHH any other byte that is not\n"
                 "# printable ASCII); bytes in hex, \"..\" filled in at run time or padding,\n"
                 "# \"++++++++\"/\"--------\" a code offset set/zero. Generated, do not edit.\n",
            base ? base + 1 : path);
    for (i = 0; i < b.n && !rc; i++)
        if (blitzlib_decls_write(b.h[i], out, err, sizeof err)) {
            fprintf(stderr, "%s: %s: library %zu: %s\n", me, path, i + 1, err);
            rc = 1;
        }
    if (fclose(out) || rc || rename(tmp.c_str(), output)) {
        if (!rc)
            fprintf(stderr, "%s: cannot write %s\n", me, output);
        remove(tmp.c_str());
        rc = 1;
    }
    free_biglib(&b);
    return rc;
}

void cmd_decls(CLI::App &app, int &status)
{
    struct options {
        std::string file, output;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand(
        "decls", "the declarations of every library of a BigLib file (or of a library file) as text");
    sub->add_option("FILE", o->file, "the BigLib or library file")->required();
    sub->add_option("-o", o->output, "the declaration data to write")->required();
    sub->footer("Writes what compare compares (no code): the data compare takes as ORIGINAL. "
                "Exit status 0, 1 a library does not read or a file cannot be read or written, 2 "
                "usage error.");
    sub->callback([sub, o, &status] {
        static std::string name;

        name = cli::name(*sub);
        me = name.c_str();
        status = decls(o->file.c_str(), o->output.c_str());
    });
}
