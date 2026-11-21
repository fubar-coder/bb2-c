/*
 * siteexamples - the Blitz Basic examples of the website
 * (tests/site-examples).
 *
 * The site (site/content, site/data) shows programs in fenced blocks
 *
 *     ```blitzbasic
 *     NPrint "Hello"
 *     ```
 *
 * (in a YAML block the fence is indented; the indentation of the opening
 * fence is taken off every line of the block).  A block is named by its
 * file (relative to the site directory) and its number in the file,
 * counted from 1: "data/libintros/acidlibs/sortlib.yaml#1".
 *
 * The manifest (tests/site-examples/examples.txt) lists every block once:
 *
 *     NAME MODE FILE#N [rwe=LIB...] [REASON]
 *
 * NAME: lower case letters, digits and "-", unique; it names the source
 *       (NAME.bas) and the expected output (NAME.out).
 * FILE#N: the block.  Behind it, words "rwe=LIB" name the RWE userlibs
 *       (libs/userlibs/rwe/LIB.c) the example needs loaded; none: the
 *       libraries of the compiler only.
 * MODE: "run" - compiled and run in the emulator, its output compared with
 *       the expected file; "compile" - compiled only, with a REASON (the
 *       rest of the line behind the rwe= words: why it is not run).
 * Blank lines and lines beginning with "#" are comments.
 *
 * se_check() fails for a block without an entry (a new example must be
 * classified, none is skipped silently) and for an entry without a block
 * (an example that changed or went away).
 */
#ifndef SITEEXAMPLES_H
#define SITEEXAMPLES_H

#include <stddef.h>

struct se_block {
    char *file;                         /* relative to the site directory */
    unsigned index;                     /* number in the file, from 1 */
    unsigned line;                      /* line of the opening fence */
    char *text;                         /* the lines, each with its newline */
};

struct se_blocks {
    struct se_block *v;
    size_t n, cap;
};

enum se_mode { SE_RUN, SE_COMPILE };

struct se_entry {
    char *name;
    enum se_mode mode;
    char *file;
    unsigned index;
    char *libs;                         /* the rwe= libraries, "," between them, "" none */
    char *reason;                       /* "" for run */
    unsigned line;                      /* line of the manifest */
};

struct se_manifest {
    struct se_entry *v;
    size_t n, cap;
};

/* the blocks of one file's TEXT (FILE: its name, relative to the site
   directory), added to B; 0, or -1 with ERR (a block without its closing
   fence, out of memory) */
int se_scan_text(struct se_blocks *b, const char *file, const char *text, char *err,
                 size_t errlen);

/* the manifest in TEXT; 0, or -1 with ERR ("line N: ...") */
int se_manifest_parse(struct se_manifest *m, const char *text, char *err, size_t errlen);

/* every block has exactly one entry and every entry a block; 0, or -1 with
   ERR naming the first problem */
int se_check(const struct se_manifest *m, const struct se_blocks *b, char *err, size_t errlen);

/* the block of an entry, NULL if none */
const struct se_block *se_find(const struct se_blocks *b, const struct se_entry *e);

/* how many entries have the mode */
size_t se_count(const struct se_manifest *m, enum se_mode mode);

void se_blocks_free(struct se_blocks *b);
void se_manifest_free(struct se_manifest *m);

#endif
