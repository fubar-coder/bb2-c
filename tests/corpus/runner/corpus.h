/*
 * corpus.h - one pass of the corpus check (bb2 compilerlauf/tools/
 * corpus_check.py + bb2compile.py): the same sources compiled by several
 * configurations (compiler + libraries) in Amiberry, the programs run, the
 * results compared with the first configuration (the reference).
 *
 * The pass file (written by CMake), lines KEY=VALUE, '#' comments:
 *   work=DIR          work directory (emptied first)
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
 *   defaultdbug=PATH  blitz2:defaultdbug (optional with fixed= and
 *                     regression=on: without it there is no
 *                     blitz2:defaultdbug)
 *   config=TAG|EXPECT|BLITZ2|DEFLIBS|ACIDLIBS   one per configuration; the
 *                     first is the reference (EXPECT ref; none with
 *                     fixed= or regression=on), the others:
 *                     exe = executable identical and output the same,
 *                     output = output the same
 *   source=PATH       a tokenised source (NAME.bb2), any number
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
 *                     CD, where the tests load sounds and modules from),
 *                     any number
 *   file=DEST|SRC[|TAG]
 *                     SRC copied to CORPUS:DEST (DEST relative, parent
 *                     directories made), after the files above: replaces
 *                     one of them (e.g. blitz2/deflibs) or adds one (a
 *                     library in blitz2/userlibs/DIR/), any number; with
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
 *   orig=PATH         with expect= (not with fixed= and regression=on):
 *                     the recorded observation of the original (NAME.orig,
 *                     bugorig.h).  The normal run starts no session of the
 *                     reference configuration (the first one, EXPECT ref,
 *                     it is only used to record), judges the others against
 *                     the expectation and this file, and gives BROKEN
 *                     before any session if the file matches the
 *                     expectation (3), or exit status 2 if it is missing.
 *                     With BUGORIG_RECORD=1 in the environment (corpus_select)
 *                     the run is the recording: only the reference
 *                     configuration runs and its observation is written to
 *                     PATH (exit status 0; 2 if the session failed)
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
 *                     against the expectation only.  Exit status 0 = every
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

enum corpus_expect { CORPUS_REF, CORPUS_EXE, CORPUS_OUTPUT };

struct corpus_config {
    char *tag;
    enum corpus_expect expect;
    char *blitz2, *deflibs, *acidlibs;
};

struct corpus_pass {
    char *work, *report, *ted, *launcher, *gate, *defaultdbug, *serial_same_as;
    char *expect, *wrap, *wrap_args, *runwrap, *runwrap_args, *runargs, *machine, *rcput;
    char *orig, *orig_by;               /* orig=, orig_by= (bug-orig tests) */
    int record;                         /* corpus_select: the recording run */
    int runtime_errors, serial, regression, run;   /* run: 0 = compile only (run=off) */
    unsigned timeout, parallel, stack;
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

/* the configurations a bug-orig pass (orig=) runs: the recording (record)
   keeps only the reference, the normal run drops it and the files that are
   for it only; a pass without orig= is left as it is.  Behind the check of
   corpus_parse */
void corpus_select(struct corpus_pass *p, int record);

/* after corpus_select: the normal run of a bug-orig pass (orig=) judges
   what is there.  A configuration whose compiler or libraries are missing
   (port: the CD's libraries) is not judged while another one is, and
   without blitz2:defaultdbug (the CD) the run goes without it unless it
   has runtime error checking (the debugger is part of the program: the
   run is skipped); both are noted in log (may be NULL).  The recording
   needs them (skipped) */
void corpus_skip_missing(struct corpus_pass *p, FILE *log);

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
int corpus_run(const struct corpus_pass *p, FILE *log);

#endif
