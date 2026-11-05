/*
 * covmap.h - the map from functions to the tests that executed them
 * (tools/covmap, openspec change test-coverage, D6/D7).
 *
 * Inputs, all as text or bytes in memory:
 *   - host: the gcov JSON of one host test (`gcov --json-format --stdout`
 *     over the test's .gcda files: one JSON document per object, one
 *     after the other); program "host";
 *   - Amiga: per program its link map (ld -Map: the objects in the code
 *     hunk and their global symbols), optionally `nm -A` of its objects
 *     (all function symbols, the static ones too) and `objdump
 *     --dwarf=decodedline` of its objects compiled with -g (the line
 *     ranges), and per test the program's coverage file (shared/cov/
 *     covfmt.h);
 *   - `ninja -t deps` of the build: the source of each object.
 *
 * Paths are kept relative to the repository root; functions in files
 * outside it or below a dropped prefix (the build directories) are left
 * out.
 *
 * The output (map.tsv), tab-separated:
 *   # commit <sha>
 *   # program  file  function  first  last  tests
 *   PROGRAM  FILE  FUNCTION  FIRST  LAST  TEST TEST ...
 *   PROGRAM  FILE  (empty)   (empty) (empty)  TEST TEST ...
 * A function line per function (FIRST, LAST its line range, empty if not
 * known; tests separated by spaces, sorted, maybe none); after the
 * functions of a file one line with an empty function: the object line,
 * every test that executed any function of the file.
 */
#ifndef COVMAP_H
#define COVMAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "covutil.h"

struct cov_func {
    int prog, file, name;       /* ids in covmap.strs */
    int first, last;            /* line range, 0 if not known */
    int *tests;                 /* ids in covmap.tests, maybe twice */
    size_t ntests, cap;
};

struct cov_obj {
    uint32_t base, size;        /* its .text in the code hunk */
    int path;                   /* object path (strs) */
    int file;                   /* its source (strs), -1 if left out */
};

struct cov_sym {
    uint32_t addr, end;         /* the function's bytes in the code hunk */
    int func;                   /* index in covmap.funcs */
    size_t obj;                 /* index in cov_prog.objs */
};

struct cov_prog {
    int name;                   /* strs */
    uint32_t text_size;
    struct cov_obj *objs;
    size_t nobjs;
    /* function symbols, as read: from the link map (globals) and nm */
    struct cov_rawsym { uint32_t addr; size_t obj; int name; int from_nm; } *raw;
    size_t nraw, capraw;
    int have_nm;                /* nm symbols read: the map's are not used */
    struct cov_sym *syms;       /* sorted by addr, after covmap_prog_ready */
    size_t nsyms;
    int ready;
};

struct covmap {
    char *root;
    char **drops;               /* path prefixes left out (relative to root) */
    size_t ndrops;
    struct strtab strs;
    struct strtab tests;
    struct cov_func *funcs;
    size_t nfuncs, cap;
    int *fslot;                 /* hash of funcs by (prog, file, name), -1 empty */
    size_t nfslot;
    struct cov_prog *progs;
    size_t nprogs;
    struct cov_deps deps;
};

void covmap_init(struct covmap *m, const char *root);
void covmap_free(struct covmap *m);
/* leave out files below prefix (a path, relative to the root or absolute) */
int covmap_drop(struct covmap *m, const char *prefix);

/* inputs; each returns 0, or -1 with a message in err */
int covmap_add_deps(struct covmap *m, const char *text, char *err, size_t errsz);
int covmap_add_gcov(struct covmap *m, const char *test, const char *text, size_t len,
                    char *err, size_t errsz);
int covmap_add_linkmap(struct covmap *m, const char *program, const char *text,
                       char *err, size_t errsz);
int covmap_add_objsyms(struct covmap *m, const char *program, const char *text,
                       char *err, size_t errsz);
int covmap_add_lines(struct covmap *m, const char *program, const char *text,
                     char *err, size_t errsz);
int covmap_add_cov(struct covmap *m, const char *test, const char *program,
                   const unsigned char *data, size_t len, char *err, size_t errsz);

/* write map.tsv */
int covmap_write(struct covmap *m, const char *commit, FILE *out);

#endif
