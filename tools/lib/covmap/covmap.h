/*
 * covmap.h - the coverage of each test as lcov tracefiles (tools/covmap,
 * openspec change test-coverage, D6/D7).
 *
 * Inputs, all as text or bytes in memory:
 *   - host: the gcov JSON of one host test (`gcov --json-format --stdout`
 *     over the test's .gcda files: one JSON document per object, one
 *     after the other);
 *   - Amiga: per program its link map (ld -Map: the objects in the code
 *     hunk and their global symbols), optionally `nm -A` of its objects
 *     (all function symbols, the static ones too) and `objdump
 *     --dwarf=decodedline` of its objects compiled with -g (the line
 *     ranges), and per test the program's coverage file (shared/cov/
 *     covfmt.h);
 *   - `ninja -t deps` of the build: the source of each object.
 *
 * Functions in files outside the repository root or below a dropped
 * prefix (the build directories) are left out.
 *
 * The output: one lcov tracefile (lcov 2.x) per test, and all of them in
 * one (all.info, the tests' sections in the order of their names, as lcov
 * merges tracefiles of different tests).  Per test:
 *   TN:<the test's name in lcov's form: each character but [A-Za-z0-9_] as
 *      '_' (cov_lcov_test_name); the name itself is the tracefile's path>
 * (all.info: two tests with the same lcov form share their TN:)
 * and per source file a section
 *   SF:<absolute path>
 *   FN:<first>,<last>,<name>   per function, in the order of their lines;
 *                              FN:1,<name> (no last line) if the lines are
 *                              not known; a symbol in the code whose bytes
 *                              hold no row of its object's source, in an
 *                              object with rows (data in .text such as
 *                              switch tables, assembler stubs, out-of-line
 *                              copies of header functions), is no function
 *                              of that source and is left out
 *   FNDA:<count>,<name>        gcov: its calls; coverage file: 1 if it was
 *                              entered, else 0
 *   FNF:<functions> FNH:<functions with a count above 0>
 *   BRDA:<line>,<block>,<branch>,<taken or ->  gcov only; the block is the
 *                              number of the line's entry in gcov's list
 *   BRF:<branches> BRH:<branches taken>
 *   DA:<line>,<count>          gcov only
 *   LF:<lines> LH:<lines with a count above 0>
 *   end_of_record
 * A test lists every function of the programs it ran (the coverage files)
 * or of the objects gcov names for it, executed or not; counts of the same
 * function in several inputs of a test add up (coverage files: the
 * largest, 1).
 */
#ifndef COVMAP_H
#define COVMAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "covutil.h"

struct cov_func {
    int file, name;             /* ids in covmap.strs */
    int first, last;            /* line range, 0 if not known */
};

/* a function in the tracefile of a test */
struct cov_hit {
    int test, func;             /* ids in covmap.tests, index in covmap.funcs */
    int bitmap;                 /* from a coverage file: merged as the largest */
    unsigned long long count;
};

/* a line (gcov), and a branch (gcov; taken -1: its block never ran) */
struct cov_line {
    int test, file, line;
    unsigned long long count;
};
struct cov_branch {
    int test, file, line, block, branch;
    long long taken;
};

struct cov_obj {
    uint32_t base, size;        /* its .text in the code hunk */
    int path;                   /* object path (strs) */
    int file;                   /* its source (strs), -1 if left out */
    int has_lines;              /* rows of its source in the line table */
};

struct cov_sym {
    uint32_t addr, end;         /* the function's bytes in the code hunk */
    int func;                   /* index in covmap.funcs */
    size_t obj;                 /* index in cov_prog.objs */
    int has_lines;              /* rows of the object's source in its bytes */
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
    int counted;                /* a coverage file read: no more lines */
};

struct covmap {
    char *root;
    char **drops;               /* path prefixes left out (relative to root) */
    size_t ndrops;
    struct strtab strs;
    struct strtab tests;
    struct cov_func *funcs;
    size_t nfuncs, cap;
    int *fslot;                 /* hash of funcs by (file, name), -1 empty */
    size_t nfslot;
    struct cov_prog *progs;
    size_t nprogs;
    struct cov_deps deps;
    struct cov_hit *hits;
    size_t nhits, caphits;
    struct cov_line *lines;
    size_t nlines, caplines;
    struct cov_branch *branches;
    size_t nbranches, capbranches;
    int done;                   /* sorted and merged for writing */
    int *order;                 /* the test ids in the order of their names */
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

/* a test name usable as the path of its tracefile and as its TN: (relative,
   no empty, "." or ".." component, not "all", no comma, tab or newline) */
int covmap_test_name_ok(const char *test);

/* the tests, in the order of their names (after all inputs; -1 if out of
   memory) */
long covmap_tests(struct covmap *m);
const char *covmap_test(const struct covmap *m, size_t i);
/* the tracefile of test i; of all tests (all.info); 0, or -1 if out of
   memory or a write failed */
int covmap_write_test(struct covmap *m, size_t i, FILE *out);
int covmap_write_all(struct covmap *m, FILE *out);

#endif
