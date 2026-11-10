/*
 * tedab.h - the TED A/B tests (bb2 ted-c/tools/tedab.py): test scripts
 * scripts/NAME.drv (teddrive syntax, amiga/teddrive.c) with directives in
 * comments, run in four combinations
 *
 *   R5  reference TED (ref/ted)  + blitz2 2.10 (BLITZ2/BLITZ2 of the CD)
 *   C5  TED-C                    + blitz2 2.10
 *   Cc  TED-C                    + blitz2-c
 *   Rc  reference TED            + blitz2-c
 *
 * and compared pairwise (R5-C5, Rc-Cc, R5-Rc, C5-Cc).  Two more
 * combinations are for the prev build (the previous commit,
 * cmake/prev.cmake), only run when the pass names them:
 *
 *   P5  TED of the previous commit + blitz2 2.10
 *   Pc  TED of the previous commit + blitz2 of the previous commit
 *
 * compared with TED-C (C5-P5, Cc-Pc): a commit that changes what TED does
 * shows there.  Directives:
 *   ; @source T NAME     tests/corpus/tests/NAME.bas, tokenised (tedsrc tok)
 *   ; @source E NAME     tests/ted-ab/src/NAME.bas as text (TED tokenises it
 *                        when it loads it)
 *   ; @source S NAME     tests/ted-ab/src/NAME.bb2
 *   ; @source K PATH     the CD's PATH (any case) (+ PATH.xtra)
 *   ; @source none       blitz2 without a file
 *   ; @opts K=V,...      L:blitzeditor.opts staged (tedab_opts_file)
 *   ; @font 6            decode the dumps with the 6 pixel font
 *   ; @xtra norunerr     .xtra with runtime errors / debugger off (the
 *                        source's own, else the corpus ref run's defaults)
 *   ; @combos R5 C5 ...  the combinations the test runs in (default all)
 *   ; @est SEC           estimated run time (default 5)
 *   ; @roundtrip         = @expect same FILE, @expect bak FILE.bak
 *   ; @expect KIND ARGS  checked on the host after the run (in work/; the
 *                        L kinds in L/):
 *                          same NAME          = the staged source
 *                          bak NAME           exists and = the staged source
 *                          exists NAME / absent NAME
 *                          tok NAME EXPECT    = tedsrc tok of scripts/EXPECT
 *                          corpus NAME TEST   = the executable TEST of the
 *                                             corpus ref run
 *                          tedsave NAME PATH  = what the original TED saves
 *                                             for the CD's PATH (oracle hash)
 *                          cd NAME PATH       = the CD's PATH
 *                          lexists NAME       L/NAME exists
 *                          opts NAME K=V,..   L/NAME = that option block
 *   ; @nofiles           the test writes no file (otherwise a test without
 *                        @roundtrip/@expect is refused: OK alone is no pass)
 *   ; @known REASON      a known failure (REASON e.g. "KNOWN-BOTH ..."): not
 *                        run, listed in the report as KNOWN with REASON;
 *                        run all the same when the filter names it exactly
 *   ; @fixed #NNN          (only in tests/bug-orig/ted, no pair) TED bug NNN is
 *                        fixed: the script is judged by its expectations
 *                        alone (FIXED, or REGRESSED), no recorded original
 *   ; @fixed #NNN PAIR...  TED bug NNN (the issue) is fixed: the named pairs
 *                        (R5-C5, Rc-Cc, R5-Rc, C5-Cc, C5-P5, Cc-Pc; both combinations in
 *                        @combos) must differ, FIXED; a named pair that is
 *                        the same is BROKEN (the fix does not show, the
 *                        directive is stale).  The other pairs are compared
 *                        as before; one line per issue, a pair once
 * Mostly for the bug-orig tests (tests/bug-orig/ted, pass key bugorig=1):
 *   ; @file NAME "DATA"  a file staged in work/ ("..." with \n \t \xHH,
 *                        also \x00); repeated lines append
 *   ; @source F NAME     the @file NAME is the file passed to blitz2
 *   ; @command ARGS...   the command teddrive runs instead of
 *                        "blitz2:blitz2 [FILE]" (as it is; @FILE@ = the
 *                        source in work/), e.g. "blitz2:ted" (TED alone) or
 *                        "TEDAB:tools/inject ... blitz2:blitz2 @FILE@"
 *   ; @expect data NAME "DATA"     work/NAME = DATA (repeated lines append)
 *   ; @expect stdout "TEXT"        the command's output (out/stdout) has
 *   ; @expect nostdout "TEXT"      / has not TEXT in it
 *   ; @expect log "TEXT"           teddrive's log has / has not TEXT
 *   ; @expect nolog "TEXT"
 *   ; @expect screen DUMP "TEXT"   the decoded dump DUMP (out/DUMP.txt)
 *   ; @expect noscreen DUMP "TEXT" has / has not TEXT in it
 *   ; @expect file NAME "TEXT"     work/NAME has / has not TEXT in it
 *   ; @expect nofile NAME "TEXT"
 * Postfix of a command line (a comment, teddrive ignores it, its log has it):
 *   CMD ARGS ; @ignore compilewindow
 *                        blitz2's compile window (no title, one gadget
 *                        "STOP!") is left out of the requester sequence in
 *                        this step: whether teddrive sees it depends on the
 *                        timing (0, 1 or 2 times for the same run)
 */
#ifndef TEDAB_H
#define TEDAB_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "previntended.h"
#include "teddump.h"
#include "util.h"

/* the session directory in the emulator (session layout: tools/ gate/
   gate/out/ tests/ combo/XX/blitz2/ runs/TEST/XX/{L,work,out}/, session.log,
   booted) */
#define TEDAB_VOL "TEDAB"

/* the default keymap of the session (tedprobe sets it): the scripts type
   umlauts with the German keymap (28-long) */
#define TEDAB_KEYMAP "d"

/* gate D: teddrivetest's filled rectangle (pen 3), screen coordinates */
#define TEDAB_GATE_X0 16
#define TEDAB_GATE_Y0 32
#define TEDAB_GATE_X1 47
#define TEDAB_GATE_Y1 39

enum { TEDAB_R5, TEDAB_C5, TEDAB_CC, TEDAB_RC, TEDAB_P5, TEDAB_PC, TEDAB_NCOMBOS };
extern const char *const tedab_combo_names[TEDAB_NCOMBOS];
/* the compared pairs, as combination indexes */
#define TEDAB_NPAIRS 6
extern const int tedab_pairs[TEDAB_NPAIRS][2];
/* index of a combination name, -1 = unknown */
int tedab_combo(const char *name);
/* index in tedab_pairs of a pair name "R5-C5", -1 = no compared pair */
int tedab_pair(const char *name);

struct tedab_expect {
    char kind[12];
    char *a, *b;                /* b: NULL for one-argument kinds */
    uint8_t *data;              /* data, stdout, log, screen (and no...) */
    size_t len;
};

/* a file of @file */
struct tedab_file {
    char *name;
    uint8_t *data;
    size_t len;
};

struct tedab_test {
    char *name;                 /* the file name without .drv */
    char src_kind;              /* 'T', 'E', 'S', 'K' or 0 (none) */
    char *src_arg;
    char *file;                 /* its name in work/ (NULL: none) */
    unsigned combos;            /* bit = 1 << TEDAB_R5 ... */
    int norunerr, roundtrip, nofiles, font, est;
    char *opts;                 /* @opts spec, NULL = none */
    char *known;                /* @known REASON, NULL = none */
    unsigned fixed[TEDAB_NPAIRS]; /* @fixed: the issue per tedab_pairs index, 0 =
                                   the pair must be the same */
    unsigned fixed_bug;         /* bug-orig: @fixed #NNN without a pair, 0 = an open bug */
    char *command;              /* @command, NULL = blitz2:blitz2 [FILE] */
    struct tedab_file *files;   /* @file */
    size_t nfiles;
    struct tedab_expect *expects;   /* @roundtrip's in front */
    size_t nexpects;
};

/* the directives of a script; 0 = ok, -1 = error (message in err) */
int tedab_test_parse(const char *name, const char *text, struct tedab_test *t, char *err,
                     size_t errlen);
/* the same for a script of tests/bug-orig/ted (bugorig 1): @fixed #NNN has no
   pair there (t->fixed_bug = NNN, a fixed test) and a pair is refused */
int tedab_test_parse_mode(const char *name, const char *text, struct tedab_test *t, char *err,
                          size_t errlen, int bugorig);
void tedab_test_free(struct tedab_test *t);

/* the commands of a script against teddrive's syntax; prints "line N: ..."
   to out, returns the number of errors */
int tedab_lint(const char *text, FILE *out);

/* TED's option block l:blitzeditor.opts (25 big-endian words) with the
   defaults TED writes under blitz2 and the fields of spec ("k=v,k=v");
   0 = ok, -1 = unknown field (message in err) */
int tedab_opts_file(const char *spec, uint8_t out[50], char *err, size_t errlen);

/* the requester sequence of a teddrive log: one line per requester window,
   "requester TITLE ID:LABEL ...", without addresses (and without the
   compile window in steps whose line has @ignore compilewindow); malloc'd lines
   (NULL at the end), *n of them */
/* the postfix (a comment) of a script line: 0 = none, 1 = "@ignore
   compilewindow", -1 = an unknown one (lint) */
int tedab_postfix(const char *line, size_t len);
char **tedab_requesters(const char *log, size_t *n);
void tedab_free_lines(char **lines);

/* one run of a session */
struct tedab_run {
    const struct tedab_test *test;
    int combo;
};

/* the AmigaDOS script of one session (malloc'd): reboot guard, gate 0
   (tedprobe; WARN -> the rest skipped), gate D (teddrive against
   teddrivetest, a normal script and one that fails on purpose and proves
   the rescue), then the runs - the first one always, the next test only if
   the session's first run is OK, the other combinations of a test only if
   its first one is OK */
char *tedab_session_script(const struct tedab_run *runs, size_t n, unsigned stack);
/* the run whose out/ok run i of a session waits for in the script: the
   test's first run, for that one the session's first (0, also for i = 0) */
size_t tedab_session_waits_for(const struct tedab_run *runs, size_t i);
/* If/EndIf balanced, no line longer than the shell takes (255); prints the
   problems, returns their number */
int tedab_check_script(const char *script, FILE *out);

/* the run states of a session log ("RUN TEST XX start|end|skipped ...");
   the other lines go to events (", " separated, may be NULL); returns the
   number of RUN lines that matched a run */
enum { TEDAB_NOTRUN, TEDAB_STARTED, TEDAB_ENDED, TEDAB_SKIPPED };
int tedab_session_log(const char *text, const struct tedab_run *runs, size_t n, int *state,
                      struct text *events);

/* rel below base, the names compared without case (malloc'd; NULL = not
   there) */
char *tedab_resolve(const char *base, const char *rel);

/* the gate results in gdir (gate0.txt, gd.*, gr.*) as report lines;
   1 = gates OK */
int tedab_eval_gate(const char *gdir, struct text *out);

/* A/B comparison of two run directories (out/ work/ L/; na, nb their
   names): result OK in both, blitz2's rc, requesters, dumps (font fh), the
   files.  0 = same, 1 = different, -1 = a result not OK; the details as
   report lines */
int tedab_compare_runs(const char *a, const char *b, const char *na, const char *nb,
                       const struct teddump_fonts *f, int fh, struct text *out);

/* the verdict of pair k (tedab_pairs index) of test t from compare (the
   result of tedab_compare_runs): SAME or DIFF, FAIL (a result not OK); for a
   pair of @fixed FIXED (it differs) or BROKEN (it is the same: stale) */
enum { TEDAB_PAIR_SAME, TEDAB_PAIR_DIFF, TEDAB_PAIR_FAIL, TEDAB_PAIR_FIXED, TEDAB_PAIR_BROKEN,
       TEDAB_PAIR_INTENDED };
int tedab_pair_verdict(const struct tedab_test *t, int k, int compare);
/* The verdict of a pair with the prev build (C5-P5, Cc-Pc) against the
   PREV_INTENDED entries (previntended.h): a DIFF listed there (ted_ab:CASE,
   ted_ab:CASE:PAIR) or with ON is INTENDED (passes), other verdicts and
   pairs stay as they are.  Marks the entries judged. */
int tedab_pair_prev(struct prevint *pi, const struct tedab_test *t, int k, int verdict);
/* 1 = the verdict passes (SAME, FIXED, INTENDED) */
int tedab_pair_passes(int verdict);
const char *tedab_pair_verdict_name(int verdict);

/* where the expectations get their wanted contents */
struct tedab_sources {
    const char *combo;          /* for the report lines */
    const char *staged;         /* the staged source (same, bak) */
    const char *expect_dir;     /* tok: EXPECT.bb2 = tedsrc tok of scripts/EXPECT */
    const char *corpus_out;     /* corpus: the executables of the corpus ref run */
    const char *cd;             /* cd */
    const char *oracle;         /* tedsave: the TED oracle manifest */
};
/* the expectations of test t in run directory rundir; returns the number of
   problems (report lines to out) */
int tedab_check_expects(const struct tedab_test *t, const char *rundir,
                        const struct tedab_sources *src, struct text *out);

/* bug-orig verdict (the exit status of corpusrun, bugorig.h) of a port run
   against the reference run: each run's observation is its state (state
   text of the report: OK, FAIL step N ..., TIMEOUT ...) and the problems of
   its expectations (report lines without the combination name); compare =
   tedab_compare_runs of the two (only used when both are OK).
   FIXED: the port is OK without problems; BROKEN: the reference is (the
   original does not show the bug, or the expectation describes it);
   REPRODUCED: the port shows what the reference shows; DIFFERS: neither */
enum { TEDAB_FIXED = 0, TEDAB_REPRODUCED = 1, TEDAB_BROKEN = 3, TEDAB_DIFFERS = 4,
       TEDAB_REGRESSED = 5 };
int tedab_bugorig_verdict(const char *ref_state, const char *ref_problems,
                          const char *port_state, const char *port_problems, int compare);
/* the verdict of a fixed bug-orig test (t->fixed_bug, no recorded original)
   of a port run: FIXED = OK without problems, else REGRESSED (the bug shows
   again; the runner's exit status is 1 like REPRODUCED) */
int tedab_fixed_verdict(const char *port_state, const char *port_problems);
const char *tedab_verdict_name(int verdict);

/* the recorded observation of the original (NAME.orig of a TED bug-orig
   test, the R5 run): an optional note "# NOTE", then
     [state]
     the state text (one line)
     [problems]
     the problem lines, as tedab_check_expects wrote them (may be empty)
   The verdict against it is tedab_bugorig_verdict with the file's state and
   problems; the comparison of the whole run directories (compare) is not
   part of the observation.  format: 0 = ok (t malloc'd), -1 = error (err);
   parse: 0 = ok (state and problems malloc'd), -1 = error */
int tedab_orig_format(const char *state, const char *problems, const char *note, struct text *t,
                      char *err, size_t errlen);
int tedab_orig_parse(const char *text, char **state, char **problems, char *err, size_t errlen);

/* the hash of save/CDPATH (any case) in the oracle manifest; 0 = found */
int tedab_oracle_hash(const char *manifest, const char *cdpath, char hex[65]);

#endif
