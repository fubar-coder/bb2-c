/*
 * corpus.h - one pass of the corpus check (bb2 compilerlauf/tools/
 * corpus_check.py + bb2compile.py): the same sources compiled by several
 * configurations (compiler + libraries) in Amiberry, the programs run, the
 * results compared with the first configuration (the reference).
 *
 * The pass file (written by CMake), lines KEY=VALUE, '#' comments:
 *   work=DIR          work directory: every run works in a directory of
 *                     its own below it, DIR/run-XXXXXX (runwork.h; two
 *                     runs of the pass at the same time are independent),
 *                     DIR/TAG the configuration's files; a run with a
 *                     verdict becomes DIR/last (the run before it is
 *                     removed), the fixed path of the results for other
 *                     tests; a run with another exit status than 0 prints
 *                     its directory (the files stay until a later run
 *                     replaces them); a run without a verdict (exit status
 *                     2 or 77) is not made DIR/last
 *   min_free=MIB      less free space (MiB) in DIR before or after the
 *                     sessions ends the run with exit status 2 ("no space
 *                     left in DIR") instead of a verdict (default
 *                     RUNWORK_MIN_FREE_MIB, 0 = no check)
 *   report=PATH       the report (also to stdout)
 *   runtime_errors=on|off
 *                     runtime error checking of the sources (no .xtra: the
 *                     compiler's start options, with off without runtime
 *                     errors and debugger - tedstub runerr=off)
 *   timeout=SEC       per emulator session (boot, compiles, runs)
 *   parallel=N        emulator sessions at the same time (1 always works)
 *   stack=N           Stack for blitz2 and the programs
 *   ted=PATH          tedstub (blitz2:ted)
 *   launcher=PATH     starts every program: reqtrap or reqclose (not
 *                     needed with run=off)
 *   run=on|off        off: the programs are only compiled, not run (no
 *                     launcher, gate, serial, runwrap, runargs, expect);
 *                     every configuration must give the reference's
 *                     compile result (tedstub's RESULT line: ok and the
 *                     size, or the error with line and message), and with
 *                     exe the same executable - also for a source the
 *                     reference does not compile (blitz2-ab3 against
 *                     blitz2, tests/ab3)
 *   gate=PATH        optional: run through the launcher first (reqclosetest);
 *                     if it does not return 0, the programs are not run and
 *                     the pass fails (they would block on their requesters)
 *   defaultdbug=PATH  optional: blitz2:defaultdbug (the debugger module
 *                     of COMPILE/RUN; never part of an executable, so
 *                     the passes of the preset all name none)
 *   config=TAG|EXPECT|BLITZ2|DEFLIBS|ACIDLIBS   one per configuration; the
 *                     first is the reference (EXPECT ref; none with
 *                     fixed= or regression=on), the others:
 *                     exe = executable identical and output the same,
 *                     output = output the same;
 *                     exe:BASE and output:BASE compare with the
 *                     configuration BASE (another one of the pass)
 *                     instead of the reference, e.g. prev|exe:clib: the
 *                     build of the previous commit writes the executable
 *                     the current compiler with the C libraries writes.
 *                     With expect_dir= the first configuration is judged
 *                     against the files like the others and may be the only
 *                     one (the pass without the original: clib, prev)
 *   expect_dir=DIR    the committed expected data (tests/corpus/expect/
 *                     PASS, tools/lib/corpusexpect): DIR/NAME.out is what
 *                     source NAME writes (its NAME.run, rc=N behind it),
 *                     with serial=on DIR/NAME.serial the bytes it sends over
 *                     the serial port; a program a fix makes differ from
 *                     the original has the pair NAME.orig.EXT (the
 *                     original's) and NAME.fixed-N.EXT (the port's) instead,
 *                     an executable the pair NAME.orig.exe / NAME.fixed-N.exe
 *                     (SHA-256).  The output (and serial data) of every
 *                     configuration, the reference's too, is compared with
 *                     the files that apply to it (by its tag: ref the
 *                     unqualified or [orig] one, port and orig-clib either,
 *                     the others the unqualified or [fixed #N] one)
 *                     instead of the reference's (or BASE's); the
 *                     executables are compared as before, except where a
 *                     program has an exe pair: the reference's (first
 *                     configuration, tag ref) against NAME.orig.exe, a
 *                     configuration with EXPECT exe against
 *                     NAME.fixed-N.exe.  A malformed directory or a missing
 *                     file is an error before any session (exit status 2).
 *                     Not with run=off or expect=.  corpus_update writes the
 *                     files instead (corpusrun pass --update-expect)
 *   source=PATH       a tokenised source (NAME.bb2), any number
 *   test=NAME         the ctest name of the pass (the TEST of PREV_INTENDED
 *                     entries, lib/previntended)
 *   intended=ENTRY    a PREV_INTENDED entry (TEST:NAME, or ON; any number;
 *                     entries of other tests are ignored): the sources
 *                     named may differ in the configuration tagged prev,
 *                     all of them with ON; each listed one must differ
 *                     (else STALE, the pass fails), an unlisted difference
 *                     fails; needs test=
 *   serial=on|off     on: the emulated serial port is a TCP server, a
 *                     remote end answers the programs (serial.h); the bytes
 *                     a program sends over the serial port (NAME.serial) are
 *                     compared as well, and a TIMEOUT in an output counts as
 *                     a difference
 *   serial_same_as=DIR  what the reference sent over the serial port must
 *                     also equal DIR/NAME.serial (the other pass, runtime
 *                     error checking off/on); if that file is missing, only
 *                     noted
 *   mount=NAME|DIR    a read-only volume for the programs (e.g. BB2: = the
 *                     CD), any number
 *   file=DEST|SRC[|TAG]
 *                     SRC copied to CORPUS:DEST (DEST relative, parent
 *                     directories made), after the files above: replaces
 *                     one of them (e.g. blitz2/deflibs) or adds one (a
 *                     library in blitz2/userlibs/DIR/, a file the programs
 *                     load in data/), any number; with
 *                     TAG only for that configuration (e.g. the reference
 *                     of a library for ref, its C build for the others)
 *   dir=DEST          an empty directory CORPUS:DEST, any number
 *   remove=DEST       CORPUS:DEST removed after staging (e.g. blitz2/ted,
 *                     blitz2/defaultdbug), any number
 *   wrap=PATH[|ARGS]  the compiler is started through PATH (CORPUS:wrap
 *                     ARGS blitz2:blitz2 CORPUS:jobs), e.g. with faults
 *                     injected (tests/bug-orig/amiga/inject.c)
 *   runwrap=PATH[|ARGS]
 *                     every program is started through PATH, inside the
 *                     launcher (CORPUS:run CORPUS:runwrap ARGS
 *                     CORPUS:out/NAME), e.g. inject without free signals
 *   runargs=ARGS      arguments for every program (CORPUS:out/NAME ARGS,
 *                     a CLI start with arguments)
 *   jobline=LINE      a line appended to tedstub's job list (behind the
 *                     job= lines), any number
 *   uae=KEY=VALUE     a change of the emulator configuration
 *                     (tools/lib/amiga_emulation/default.uae), e.g. more
 *                     memory; any number
 *   rtg=on|off        on: the RTG variant of the emulator (the uaegfx board,
 *                     the Workbench hardfile with its monitor,
 *                     amiga_emulation_rtg), e.g. for a screen taller than
 *                     the native modes; applied before the uae= changes
 *   machine=NAME      the emulated machine (amiga_emulation.h: a1200, the
 *                     default; a500 = 68000 with Kickstart 1.3; a500-68010);
 *                     uae= changes its template.  On Kickstart 1.x the
 *                     session script is in AmigaDOS 1.3 syntax (rcput
 *                     instead of $RC and >>), there is no gate=, and the
 *                     launcher, wrap= and runwrap= must run there
 *   rcput=PATH        tests/corpus/amiga/rcput.c, staged as CORPUS:rcput
 *                     on a Kickstart 1.x machine (needed there, ignored
 *                     otherwise)
 *   expect=PATH       bug-orig test (bugorig.h): exactly one source, no
 *                     serial_same_as (serial=on gives the program the
 *                     remote end; what it sends is not judged); instead
 *                     of comparing the configurations with the reference,
 *                     each is judged against the expectation PATH (the
 *                     correct behaviour) and the observation of the
 *                     original (orig=).  Exit status
 *                     0 = every configuration FIXED, 1 = REPRODUCED (the
 *                     bug, as intended), 3 = BROKEN (the original does not
 *                     show the bug), 4 = DIFFERS (the port does not
 *                     reproduce the original)
 *   orig=PATH         with expect= (not with regression=on): the recorded
 *                     observation of the original (NAME.orig, bugorig.h);
 *                     with fixed= only checked against the expectation
 *                     before any session (BROKEN, 3: a fixed test keeps
 *                     its recorded original, the pair of the shared test
 *                     model).  The normal run starts no session of the
 *                     reference configuration (the first one, EXPECT ref,
 *                     it is only used to record), judges the others against
 *                     the expectation and this file, and gives BROKEN
 *                     before any session if the file matches the
 *                     expectation (3), or exit status 2 if it is missing
 *                     or breaks the rule of its wildcard lines (bugorig.h)
 *   record=check      with orig=: the compare test of the recorded original
 *                     (ctest bug_orig_record_ID, label originals): only the
 *                     reference configuration runs (corpus_select) and its
 *                     observation is compared with orig= (its wildcard lines
 *                     included, corpus_record_check); exit status 0 = it
 *                     matches, 1 = it differs (the report names the section
 *                     and the first differing line), 2 = the session failed
 *                     or orig= is missing or malformed.  No file of the
 *                     source tree is written
 *   record=write      with orig= and record_to=PATH: the recording run of
 *                     the target bug-orig-record: only the reference runs
 *                     and its observation is written to PATH (in the build
 *                     directory; results bug-orig-accept makes orig= from
 *                     two of them); exit status 0, 2 if the session failed
 *   layout=stack      with record=write: another memory layout for the
 *                     second recording run (corpus_layout): the Stack of
 *                     the compiler and the program CORPUS_LAYOUT_STACK
 *                     bytes larger, so that what they allocate lies
 *                     elsewhere; nothing else of the run changes
 *   orig_by=TEXT      the note in the first line of the recorded file
 *                     (which original it is)
 *   regression=on     with expect=: an expect test instead (an extension of
 *                     blitz2, tests/blitz2; a port regression test,
 *                     tests/bug-orig/regression): no reference, every
 *                     configuration (none with EXPECT ref) is judged
 *                     against the expectation only, as with fixed=.  Exit
 *                     status 0 = every configuration FIXED (it shows the
 *                     expected behaviour), 1 = one REGRESSED (it does not)
 *   fixed=NNN         with expect=: a fixed bug-orig test (the bug is fixed
 *                     in the port, issue NNN): no reference, every
 *                     configuration (none with EXPECT ref) is judged
 *                     against the expectation only (orig= is checked
 *                     before, see there).  Exit status 0 = every
 *                     configuration FIXED, 1 = one REGRESSED (it does not
 *                     show the expected behaviour)
 * With runtime error checking the 8 byte parameter blocks behind trap #1
 * and trap #4 are zeroed before comparing (the memory address of the
 * editor's line item, depends on the memory layout).
 */
#ifndef CORPUS_H
#define CORPUS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "corpusexpect.h"
#include "previntended.h"

enum corpus_expect { CORPUS_REF, CORPUS_EXE, CORPUS_OUTPUT };
enum corpus_record { CORPUS_RECORD_NONE, CORPUS_RECORD_CHECK, CORPUS_RECORD_WRITE };
enum { CORPUS_LAYOUT_STACK = 16384 };     /* layout=stack: bytes more of Stack */
/* corpus_update: the outputs from the configuration, or the executables
   of the reference and the configuration */
enum corpus_update { CORPUS_UPDATE_NONE, CORPUS_UPDATE_OUT, CORPUS_UPDATE_EXE };

struct corpus_config {
    char *tag;
    enum corpus_expect expect;
    char *base;                         /* exe:BASE, output:BASE: the tag of the configuration
                                           compared with instead of the reference, or NULL */
    char *blitz2, *deflibs, *acidlibs;
};

struct corpus_pass {
    char *work, *report, *ted, *launcher, *gate, *defaultdbug, *serial_same_as;
    char *expect, *wrap, *wrap_args, *runwrap, *runwrap_args, *runargs, *machine, *rcput;
    char *orig, *orig_by;               /* orig=, orig_by= (bug-orig tests) */
    char *test;                         /* test=: the ctest name (PREV_INTENDED entries) */
    char *expect_dir;                   /* expect_dir=: the expected outputs */
    struct cexp_set expected;           /* the files of expect_dir= (corpus_load_expect,
                                           corpus_update) */
    int update;                         /* corpus_update: enum corpus_update */
    struct cexp_fix *fix;               /* corpus_update: the FIX entries */
    size_t nfix;
    char *observe_to;                   /* corpus_observe: the directory of the observations */
    struct prevint intended;            /* intended=: PREV_INTENDED, for the config "prev" */
    int record;                         /* record=: enum corpus_record */
    char *record_to;                    /* record_to= (record=write) */
    int layout_stack;                   /* layout=stack */
    int runtime_errors, serial, regression, run;   /* run: 0 = compile only (run=off) */
    int rtg;                            /* rtg=on: amiga_emulation_rtg */
    unsigned timeout, parallel, stack;
    unsigned long long min_free;        /* min_free=: MiB, 0 = no check */
    unsigned fixed;                     /* fixed=NNN, 0 = not given */
    struct corpus_config *configs;
    size_t nconfigs;
    char **sources;
    size_t nsources;
    char **mount_name, **mount_dir;
    size_t nmounts;
    char **file_dest, **file_src, **file_tag;   /* file_tag[i] NULL: every configuration */
    size_t nfiles;
    char **dirs, **removes, **joblines;
    size_t ndirs, nremoves, njoblines;
    char **uae;                         /* KEY=VALUE */
    size_t nuae;
};

/* 0 = ok, -1 = error (message in err) */
int corpus_parse(const char *text, struct corpus_pass *p, char *err, size_t errlen);
void corpus_free(struct corpus_pass *p);

/* the configurations a bug-orig pass (orig=) runs: the compare test of the
   recorded original (record=) keeps only the reference, the normal run
   drops it and the files that are for it only; a pass without orig= is
   left as it is.  Behind the check of corpus_parse */
void corpus_select(struct corpus_pass *p);

/* layout=stack: the Stack of the session CORPUS_LAYOUT_STACK bytes larger
   (noted in log, may be NULL); behind corpus_select.  0 = ok */
int corpus_layout(struct corpus_pass *p, FILE *log);

/* record=check: the observation of the original (obs, the text of an
   observation file, bugorig_format) against the recorded original in the
   file orig (read only, never written); 0 = it matches, 1 = it differs
   (msg: "[SECTION] line N: recorded \"...\", observed \"...\""), 2 = orig
   is missing, unreadable or malformed (msg) */
int corpus_record_check(const char *orig, const char *obs, char *msg, size_t msglen);

/* after corpus_select: the normal run of a bug-orig pass (orig=) judges
   what is there.  A configuration whose compiler or libraries are missing
   (port: the CD's libraries) is not judged while another one is, and
   without blitz2:defaultdbug (the CD) the run goes without it, also with
   runtime error checking (an executable links runerrlib and rundebuglib,
   never the module); both are noted in log (may be NULL).  The recording
   needs them (skipped).

   A pass that compares with expect_dir= (not a bug-orig pass, not the
   update) goes on without the CD in the same way: without
   blitz2:defaultdbug, and a source
   whose text names a mount (BB2:) whose directory is missing is left out
   (noted in log); no source left: skipped */
void corpus_skip_missing(struct corpus_pass *p, FILE *log);

/* the update of the expected data (expect_dir=, corpusrun pass
   --update-expect TAG [--fix PASS:NAME#N]...): the files of expect_dir=
   are read (a malformed directory is an error), the fixes kept for
   corpus_run.  If TAG is a configuration compared by its executable with
   the reference (EXPECT exe, no BASE: port), the reference and TAG run and
   corpus_run writes the executables (cexp_update_exe) of the sources that
   have an exe pair or a fix of this pass (the others are left out; none:
   nothing to update, exit status 0); else only TAG runs and corpus_run
   writes its outputs (cexp_update: a changed unqualified file is refused
   unless a fix names the program, which makes it the pair).  The pass is
   the name of expect_dir=.  0 = ok, -1 = no expect_dir=, no configuration
   TAG, a malformed directory (message in err).  Behind the check of
   corpus_parse */
int corpus_update(struct corpus_pass *p, const char *tag, const struct cexp_fix *fix,
                  size_t nfix, char *err, size_t errlen);

/* the observation of one configuration (corpusrun pass --observe TAG --to
   DIR, the target corpus-expect-record): only TAG runs, and corpus_run
   writes for every source DIR/NAME.out (its output, rc=N behind it), with
   serial=on DIR/NAME.serial and DIR/NAME.exe (the SHA-256 of its
   executable, cexp_exe_hash; with runtime error checking the trap blocks
   zeroed); a source that does not compile gets no files.  The directory is
   emptied first; the run's work directory is DIR.work and its report
   DIR.txt (not work= and report=: two observations of one pass run at
   the same time).  layout_stack: the second run of the record (corpus_layout).
   0 = ok, -1 = no configuration TAG (err).  Behind the check of
   corpus_parse */
int corpus_observe(struct corpus_pass *p, const char *tag, const char *dir, int layout_stack,
                   char *err, size_t errlen);

/* only the sources named (NAME without .bb2) are run: 0 = ok, -1 = a name
   that is no source of the pass (err) */
int corpus_only(struct corpus_pass *p, const char *const *names, size_t n, char *err,
                size_t errlen);

/* expect_dir=: read the expected data into p->expected (strict: no lone file
   of a pair) and check that every source has its output file (and with
   serial=on its serial file); 0 = yes (or no expect_dir=), -1 = a
   malformed directory or a missing file (err names it) */
int corpus_load_expect(struct corpus_pass *p, char *err, size_t errlen);

/* expect_dir=: the path of the expected file of source i (".out" or
   ".serial"; malloc'd, NULL if out of memory) */
char *corpus_expect_path(const struct corpus_pass *p, size_t i, const char *ext);

/* the output got (glen bytes, NULL = none) against the expected want:
   cexp_diff (corpusexpect.h) */
int corpus_expect_diff(const uint8_t *want, size_t wlen, const uint8_t *got, size_t glen,
                       char *what, size_t len);

/* the name of a source without directory and .bb2 (malloc'd) */
char *corpus_name(const char *source);

/* zero the 8 bytes behind every trap #1 ($4E41) and trap #4 ($4E44) */
void corpus_mask_traps(uint8_t *data, size_t len);

/* the session script of the pass (malloc'd, NULL if out of memory):
   compile with tedstub's job list, run every program through the
   launcher, rc=N behind the outputs; AmigaDOS 1.3 syntax on a Kickstart
   1.x machine */
char *corpus_script(const struct corpus_pass *p);

/* runs the pass: 0 = no differences, 1 = differences (report), 77 = the
   emulator or a file of the pass is missing (skipped), 2 = error; with
   expect= the bug-orig exit status above */
int corpus_run(struct corpus_pass *p, FILE *log);

#endif
