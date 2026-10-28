/*
 * corpus.h - one pass of the corpus check (bb2 compilerlauf/tools/
 * corpus_check.py + bb2compile.py): the same sources compiled by several
 * configurations (compiler + libraries) in Amiberry, the programs run, the
 * results compared with the first configuration (the reference).
 *
 * The pass file (written by CMake), lines KEY=VALUE, '#' comments:
 *   work=DIR          work directory (emptied first)
 *   report=PATH       the report (also to stdout)
 *   runerr=on|off     runtime error checking of the sources (no .xtra:
 *                     the compiler's start options, with off without
 *                     runtime errors and debugger - tedstub runerr=off)
 *   timeout=SEC       per emulator session (boot, compiles, runs)
 *   parallel=N        emulator sessions at the same time (1 always works)
 *   stack=N           Stack for blitz2 and the programs
 *   ted=PATH          tedstub (blitz2:ted)
 *   runner=PATH       runs every program: reqtrap or reqclose
 *   gate=PATH         optional: run through the runner first (reqclosetest);
 *                     if it does not return 0, the programs are not run and
 *                     the pass fails (they would block on their requesters)
 *   defaultdbug=PATH  blitz2:defaultdbug
 *   config=TAG|EXPECT|BLITZ2|DEFLIBS|ACIDLIBS   one per configuration; the
 *                     first is the reference (EXPECT ref), the others:
 *                     exe = executable identical and output the same,
 *                     output = output the same
 *   source=PATH       a tokenised source (NAME.bb2), any number
 *   mount=NAME|DIR    a read-only volume for the programs (e.g. BB2: = the
 *                     CD, where the tests load sounds and modules from),
 *                     any number
 * With runtime error checking the 8 byte parameter blocks behind trap #1
 * and trap #4 are zeroed before comparing (the memory address of the
 * editor's line item, depends on the memory layout).
 */
#ifndef CORPUS_H
#define CORPUS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum corpus_expect { CORPUS_REF, CORPUS_EXE, CORPUS_OUTPUT };

struct corpus_config {
    char *tag;
    enum corpus_expect expect;
    char *blitz2, *deflibs, *acidlibs;
};

struct corpus_pass {
    char *work, *report, *ted, *runner, *gate, *defaultdbug;
    int runerr;
    unsigned timeout, parallel, stack;
    struct corpus_config *configs;
    size_t nconfigs;
    char **sources;
    size_t nsources;
    char **mount_name, **mount_dir;
    size_t nmounts;
};

/* 0 = ok, -1 = error (message in err) */
int corpus_parse(const char *text, struct corpus_pass *p, char *err, size_t errlen);
void corpus_free(struct corpus_pass *p);

/* the name of a source without directory and .bb2 (malloc'd) */
char *corpus_name(const char *source);

/* zero the 8 bytes behind every trap #1 ($4E41) and trap #4 ($4E44) */
void corpus_mask_traps(uint8_t *data, size_t len);

/* runs the pass: 0 = no differences, 1 = differences (report), 77 = the
   emulator or a file of the pass is missing (skipped), 2 = error */
int corpus_run(const struct corpus_pass *p, FILE *log);

#endif
