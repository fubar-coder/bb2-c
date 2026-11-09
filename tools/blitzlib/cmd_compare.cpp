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
        || !(b->buf = (uint8_t *)malloc((size_t)size + 1)) || fread(b->buf, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "%s: cannot read %s\n", me, path);
        if (f)
            fclose(f);
        return -1;
    }
    fclose(f);
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
    if (a.n != b.n) {
        printf("%s: %zu libraries, %s: %zu\n", original, a.n, changed, b.n);
        diffs++;
    }
    n = a.n < b.n ? a.n : b.n;
    for (i = 0; i < n; i++) {
        char name[32], *msgs;
        FILE *f;
        int d;

        if (libnum(&a.lf[i]) != libnum(&b.lf[i])) {
            printf("library %zu: number %u != %u (order of the libraries)\n", i + 1,
                   libnum(&a.lf[i]), libnum(&b.lf[i]));
            diffs++;
            continue;
        }
        snprintf(name, sizeof name, "lib %u", libnum(&a.lf[i]));
        if (!(f = tmpfile())) {
            fprintf(stderr, "%s: no temporary file\n", me);
            return 1;
        }
        d = blitzlib_compare(&a.lf[i].hunks[0], &b.lf[i].hunks[0], name, f);
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
    sub->add_option("ORIGINAL", o->original, "the original BigLib or library file")->required();
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
