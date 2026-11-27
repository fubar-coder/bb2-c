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
 *                     DIR/TAG the configuration's files; a run that
 *                     passes (exit status 0) leaves nothing (with
 *                     publish= it becomes a fixture, below), a run with
 *                     another exit status prints its directory (the files
 *                     stay until a later run of the pass replaces them)
 *   publish=NAME      a run that passes becomes the fixture NAME of the
 *                     run directory, RUN/NAME (rundir.h): the tests that
 *                     read it name RUN/NAME/TAG/vol/out as their input
 *   run_dir=DIR       the run directory RUN when the environment variable
 *                     BB2C_TEST_RUN is not set (rundir.h); publish= needs
 *                     one of them: no session starts without
 *   min_free=MIB     less free space (MiB) in DIR before or after the
 *                     sessions ends the run with exit status 2 ("no space
 *                     left in DIR") instead of a verdict (default
 *                     RUNWORK_MIN_FREE_MIB, 0 = no check)
 *   report=PATH       the report (also to stdout)
 *   runtime_errors=on|off
 *                     runtime error checking of the sources (no .xtra: the
 *                     compiler's start options, with off without runtime
 *                     errors and debugger - tedstub runerr=off)
 *   timeout=SEC       per emulator session (boot, compiles, runs)
 *   hang_after=SEC    a session whose job (the compile, the programs) has changed
 *                     no file of the volumes for SEC seconds is ended as a
 *                     timeout (amiga_emulation_params.hang_after: a program
 *                     that is expected to hang, e.g. the original's bug, need not
 *                     wait for the whole timeout; not set: only the timeout)
 *   hang_from=run     with hang_after=: the silence counts only from the first
 *                     program run (the file the gate or the first program
 *                     creates, amiga_emulation_params.hang_arm), not from the
 *                     start of the job: the compile is silent for long on a loaded
 *                     host and is no hang.  A program that hangs or crashes ends
 *                     its session after hang_after seconds; a compile that does
 *                     not end still takes the whole timeout
 *   parallel=N        emulator sessions at the same time (1 always works)
 *   stack=N           Stack for blitz2 and the programs
 *   ted=PATH          tedstub (blitz2:ted)
 *   launcher=PATH     starts every program: reqtrap or reqclose (not
 *                     needed with run=off)
 *   run=on|off        off: the programs are only compiled, not run (no
 *                     launcher, gate, serial, runwrap, runargs, launcherargs, expect);
 *                     every configuration must give the reference's
 *                     compile result (tedstub's RESULT line: ok and the
 *                     size, or the error with line and message), and with
 *                     exe the same executable - also for a source the
 *                     reference does not compile (blitz2-ab3 against
 *                     blitz2, tests/ab3)
 *   mask_traps=on    the trap #1 / #4 parameter blocks of the executables are zeroed before they
 *                     are compared, as with runtime_errors=on (a compile whose options turn
 *                     runtime error checking on without the pass saying so)
 *   must_compile=on   with run=off: a source the reference does not compile
 *                     fails the pass (the report says "must compile" and
 *                     the compiler's result): a pass that only checks that
 *                     the sources compile (tests/site-examples; the reference
 *                     alone is a pass then)
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
 *                     exeinfo = output the same, the executable of the
 *                     reference only compared for information (a different
 *                     one is reported, none at all fails: the port against
 *                     the original, whose executables are not compared),
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
 *                     original's) and NAME.fixed-N.EXT (the port's) instead
 *                     (an executable is no observation: a NAME.exe file is
 *                     refused).  The output (and serial data) of every
 *                     configuration, the reference's too, is compared with
 *                     the files that apply to it (by its tag: ref the
 *                     unqualified or [orig] one, port and orig-clib either,
 *                     the others the unqualified or [fixed #N] one)
 *                     instead of the reference's (or BASE's); the
 *                     executables are compared as before (exeinfo: for
 *                     information).  A malformed directory or a missing
 *                     file is an error before any session (exit status 2).
 *                     Not with run=off or expect=.  corpus_update writes the
 *                     files instead (corpusrun pass --update-expect)
 *   source=PATH[|NAME]
 *                     a tokenised source (NAME.bb2), any number; with NAME
 *                     (a path inside the volume, e.g. ID/child.bas: the
 *                     data source of a test of the BASIC test model in the
 *                     comparison with the prev build) its name in the report
 *                     and the PREV_INTENDED entries (its files in the volume
 *                     are src/nNNN.bb2 and out/nNNN, NNN its number among
 *                     the sources)
 *   test=NAME         the ctest name of the pass (the TEST of PREV_INTENDED
 *                     entries, lib/previntended)
 *   intended=ENTRY    a PREV_INTENDED entry (TEST:NAME, or ON; any number;
 *                     entries of other tests are ignored): the sources
 *                     named may differ in the configuration tagged prev,
 *                     all of them with ON; each listed one must differ
 *                     (else STALE, the pass fails), an unlisted difference
 *                     fails; needs test=
 *   unjudged=TEST:CONFIG:NAME|REASON
 *                     with expect_dir=: the output (and serial data) of
 *                     source NAME in the configuration CONFIG is not judged
 *                     (a combination of an unfixed original bug and a fix
 *                     that no expected file of the program describes); the
 *                     report says so and gives REASON, the program's other
 *                     configurations are judged.  TEST is the ctest name of
 *                     the pass (test=), CONFIG one of its configurations,
 *                     NAME one of its sources (else an error before any
 *                     session); an entry whose program was run and whose
 *                     output is as expected again is STALE (the pass
 *                     fails); any number (tests/corpus/CMakeLists.txt,
 *                     CORPUS_UNJUDGED)
 *   serial=on|off     on: the emulated serial port is a TCP server, a
 *                     remote end answers the programs (serial.h); the bytes
 *                     a program sends over the serial port (NAME.serial) are
 *                     compared as well, and a TIMEOUT in an output counts as
 *                     a difference
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
 *   launcherargs=ARGS options of the launcher (CORPUS:run ARGS ...): reqtrap
 *                     -harden, tests/README.md "The handlers of reqtrap"
 *   jobline=LINE      a line appended to tedstub's job list (behind the
 *                     job= lines), any number
 *   uae=KEY=VALUE     a change of the emulator configuration
 *                     (tools/lib/amiga_emulation/default.uae), e.g. more
 *                     memory; any number
 *   floppy=N|SRC      the disk image SRC (e.g. @TESTDATA@/NAME.adf, made from
 *                     NAME.adf.spec) in drive DFN (0 to 3), once per drive;
 *                     the drives without one stay empty (a test of
 *                     ritrackdisklib uses DF1).  Every configuration gets its
 *                     own writable copy WORK/TAG/emu/dfN.adf and the session
 *                     boots cold (no snapshot: its key would hold the copy of
 *                     every run), so there is no hang check.  Applied before
 *                     the uae= changes (floppyNwp=true: write protected).  A
 *                     disk in DF0 without a boot block is skipped by the boot
 *   rtg=on|off        on: the RTG variant of the emulator (the uaegfx board,
 *                     the Workbench hardfile with its monitor,
 *                     amiga_emulation_rtg), e.g. for a screen taller than
 *                     the native modes; applied before the uae= changes
 *   machine=NAME      the emulated machine (amiga_emulation.h: a1200, the
 *                     default; a500 = 68000 with Kickstart 1.3; a500-68010;
 *                     a500-204 = 68000 with Kickstart 2.04; a600 = 68000
 *                     with Kickstart 3.1);
 *                     uae= changes its template.  On Kickstart 1.x the
 *                     session script is in AmigaDOS 1.3 syntax (rcput
 *                     instead of $RC and >>), there is no gate=, and the
 *                     launcher, wrap= and runwrap= must run there
 *   rcput=PATH        tests/corpus/amiga/rcput.c, staged as CORPUS:rcput
 *                     on a Kickstart 1.x machine (needed there, ignored
 *                     otherwise)
 *   expect=PATH       bug-orig test (bugorig.h): exactly one source
 *                     (serial=on gives the program the
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
 *   cell=on           a cell of a test of the BASIC test model (caserun.h):
 *                     exactly one configuration (its EXPECT ref) and one
 *                     source; the pass is only staged (corpus_stage) and its
 *                     script halves put into a session with other cells by
 *                     corpusrun case, never run by corpus_run, which judges
 *                     nothing of it.  Not with expect=, expect_dir=, orig=,
 *                     fixed=, regression=on, record=, gate=, publish=
 *   editrun=on        with cell=on: the program runs from the editor
 *                     instead of from the shell (change
 *                     basic-test-editor-run): behind the job list (its
 *                     jobs, the jobline= lines) the line
 *                     run=r001|CORPUS:src/NAME.bb2|CORPUS:out/NAME.con
 *                     (tedstub's COMPILE/RUN, the editor's console the
 *                     file NAME.con), and no run of the program in the
 *                     second script half; the cell has ended when its
 *                     blitz2.out ends with rc=N.  With hang_from=run the
 *                     hang check arms on NAME.con, which tedstub opens
 *                     when the program starts.  Not with runwrap=,
 *                     runargs=, launcherargs= (the shell start)
 *   cli=ab3|amiblitz3
 *                     with cell=on (not with editrun=, wrap=, build=,
 *                     build_job=, jobline=): the source is a text driver
 *                     (NAME.ab3, or NAME.bb for blitz2-ab3's BB2 mode; change
 *                     ab3-test-model), staged as CORPUS:src/NAME.ab3 (.bb),
 *                     and compiled from the command line instead of
 *                     tedstub's job list, after `Assign Blitz3:
 *                     CORPUS:Blitz3` (the tree the pass stages with its
 *                     file= and dir= lines): ab3 runs the configuration's
 *                     compiler, blitz2-ab3 (blitz2:blitz2), amiblitz3
 *                     AmiBlitz3's (Blitz3:Amiblitz3), each as COMPILER
 *                     >CORPUS:out/blitz2.out -s CORPUS:src/NAME.EXT -e
 *                     CORPUS:out/NAME, with -r when runtime_errors=off;
 *                     rc=N behind the output.  The program runs as any
 *                     cell's
 *   build=DIR|BLITZ2|DEFLIBS|ACIDLIBS|on|off
 *                     with cell=on: a compile part of the cell's own (the
 *                     data of a test that another build configuration
 *                     compiles, change basic-test-local-data): BLITZ2, the
 *                     ted= stub and the libraries staged in CORPUS:DIR/blitz2/
 *                     (DIR inside the volume, e.g. ID.test/.build-1), its job
 *                     list CORPUS:DIR/jobs (log CORPUS:DIR/session.log,
 *                     runtime error checking on or off), compiled before the
 *                     cell's compile with blitz2: assigned to it, the output
 *                     CORPUS:DIR/blitz2.out with rc=N behind it; any number
 *   build_job=PART|SRC|DEST
 *                     with cell=on: the tokenised source SRC staged and
 *                     compiled to CORPUS:DEST (parent directories made), as
 *                     job bNNN (NNN its number among these lines, from 000)
 *                     of the cell's job list (PART -) or of the part PART
 *                     (the DIR of a build= line); any number
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
#include "amiga_emulation.h"

#define CORPUS_FLOPPIES 4
/* room for corpus_floppy_changes: floppyN and floppyNtype per drive, nr_floppies */
#define CORPUS_FLOPPY_CHANGES (2 * CORPUS_FLOPPIES + 1)

enum corpus_expect { CORPUS_REF, CORPUS_EXE, CORPUS_EXEINFO, CORPUS_OUTPUT };
/* cli=: the command-line compile of a cell */
enum corpus_cli { CORPUS_CLI_NONE, CORPUS_CLI_AB3, CORPUS_CLI_AMIBLITZ3 };
enum corpus_record { CORPUS_RECORD_NONE, CORPUS_RECORD_CHECK, CORPUS_RECORD_WRITE };
enum { CORPUS_LAYOUT_STACK = 16384 };     /* layout=stack: bytes more of Stack */
/* corpus_update: the outputs from the configuration */
enum corpus_update { CORPUS_UPDATE_NONE, CORPUS_UPDATE_OUT };

struct corpus_config {
    char *tag;
    enum corpus_expect expect;
    char *base;                         /* exe:BASE, output:BASE: the tag of the configuration
                                           compared with instead of the reference, or NULL */
    char *blitz2, *deflibs, *acidlibs;
};

/* unjudged=TEST:CONFIG:NAME|REASON: a program not judged in a configuration */
struct corpus_unjudged {
    char *entry;                        /* TEST:CONFIG:NAME, as given */
    char *config, *name, *reason;
    int seen;                           /* judged: the program was run in CONFIG */
    int matched;                        /* every observation was as expected (STALE) */
};

struct corpus_pass {
    char *work, *report, *ted, *launcher, *gate, *defaultdbug;
    char *publish, *run_dir;            /* publish=, run_dir= (rundir.h) */
    char *expect, *wrap, *wrap_args, *runwrap, *runwrap_args, *runargs, *launcherargs;
    char *machine, *rcput;
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
    struct corpus_unjudged *unjudged;   /* unjudged= */
    size_t nunjudged;
    int record;                         /* record=: enum corpus_record */
    char *record_to;                    /* record_to= (record=write) */
    int layout_stack;                   /* layout=stack */
    int runtime_errors, serial, regression, run;   /* run: 0 = compile only (run=off) */
    int rtg;                            /* rtg=on: amiga_emulation_rtg */
    int mask_traps;                     /* mask_traps=on: the trap blocks of the executables are masked as with runtime_errors */
    int must_compile;                   /* must_compile=on (run=off): a source that does not compile fails */
    int cell;                           /* cell=on: a cell of the BASIC test model (caserun.h) */
    int editrun;                        /* editrun=on (cell=on): the program runs from the editor */
    int cli;                            /* cli= (cell=on): enum corpus_cli */
    unsigned timeout, parallel, stack, hang_after;   /* hang_after: 0 = not given */
    int hang_from_run;                  /* hang_from=run: the silence counts from the first run */
    unsigned long long min_free;        /* min_free=: MiB, 0 = no check */
    unsigned fixed;                     /* fixed=NNN, 0 = not given */
    struct corpus_config *configs;
    size_t nconfigs;
    char **sources;
    char **source_names;                /* source=PATH|NAME: NAME, NULL: corpus_name */
    size_t nsources;
    char **mount_name, **mount_dir;
    size_t nmounts;
    char **file_dest, **file_src, **file_tag;   /* file_tag[i] NULL: every configuration */
    size_t nfiles;
    char **dirs, **removes, **joblines;
    size_t ndirs, nremoves, njoblines;
    char **uae;                         /* KEY=VALUE */
    size_t nuae;
    char *floppy[CORPUS_FLOPPIES];      /* floppy=N|SRC: the image of drive DFN, NULL = none */
    struct corpus_build *builds;        /* build=: the compile parts of a cell */
    size_t nbuilds;
    struct corpus_build_job *bjobs;     /* build_job=: the data sources of a cell */
    size_t nbjobs;
};

/* build=DIR|BLITZ2|DEFLIBS|ACIDLIBS|on|off (cell=on): a compile part of its
   own before the cell's compile, its compiler in CORPUS:DIR/blitz2/ */
struct corpus_build {
    char *dir, *blitz2, *deflibs, *acidlibs;
    int runtime_errors;
};

/* build_job=PART|SRC|DEST (cell=on): the tokenised source SRC compiled to
   CORPUS:DEST, in the cell's job list (PART "-") or in the part PART (the
   DIR of a build= line); its job is bNNN, NNN its number among the
   build_job= lines */
struct corpus_build_job {
    char *part;                         /* NULL: the cell's job list */
    char *src, *dest;
};


/* WORK/TAG/emu/dfN.adf: the writable copy of the image of drive N of configuration c
   (malloc'd, NULL = out of memory) */
char *corpus_floppy_path(const struct corpus_pass *p, size_t c, unsigned unit);

/* copies every floppy= image to its path (WORK/TAG/emu must exist); 0 = ok, -1 = error
   (the message in err) */
int corpus_floppy_stage(const struct corpus_pass *p, size_t c, char *err, size_t errlen);

/* the emulator changes of configuration c for its floppy= images: floppyN = the copy,
   floppyNtype=0 for N >= 1 (the templates have DF1 off) and nr_floppies; into out
   (CORPUS_FLOPPY_CHANGES entries), the paths malloc'd into paths[unit] (the caller
   frees them, also after an error).  Returns the number of entries, 0 without floppy=,
   (size_t)-1 = out of memory.  A pass with floppy= boots cold */
size_t corpus_floppy_changes(const struct corpus_pass *p, size_t c,
                             struct amiga_emulation_option *out, char *paths[CORPUS_FLOPPIES]);
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
   corpus_run.  Only TAG runs and corpus_run writes its outputs
   (cexp_update: a changed unqualified file is refused unless a fix names
   the program, which makes it the pair).  The pass is the name of
   expect_dir=.  0 = ok, -1 = no expect_dir=, no configuration TAG, a
   malformed directory (message in err).  Behind the check of
   corpus_parse */
int corpus_update(struct corpus_pass *p, const char *tag, const struct cexp_fix *fix,
                  size_t nfix, char *err, size_t errlen);

/* the observation of one configuration (corpusrun pass --observe TAG --to
   DIR, the target corpus-expect-record): only TAG runs, and corpus_run
   writes for every source DIR/NAME.out (its output, rc=N behind it), with
   serial=on DIR/NAME.serial; a source that does not compile gets no
   files.  The directory is
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

/* the name of source i of p: the NAME of source=PATH|NAME, else
   corpus_name (malloc'd) */
char *corpus_source_name(const struct corpus_pass *p, size_t i);

/* the name of the files of source i in the volume: corpus_name, or nNNN
   (NNN = i) for a source=PATH|NAME (src/nNNN.bb2, out/nNNN: a NAME with a
   directory would meet the file of another source); malloc'd */
char *corpus_source_file(const struct corpus_pass *p, size_t i);

/* zero the 8 bytes behind every trap #1 ($4E41) and trap #4 ($4E44) */
void corpus_mask_traps(uint8_t *data, size_t len);

/* the session script of the pass (malloc'd, NULL if out of memory):
   compile with tedstub's job list, run every program through the
   launcher, rc=N behind the outputs; AmigaDOS 1.3 syntax on a Kickstart
   1.x machine */
char *corpus_script(const struct corpus_pass *p);

/* the two halves of corpus_script (malloc'd, NULL if out of memory): the
   compile with tedstub's job list, and the runs of the programs; corpus_script
   is the one followed by the other.  The runner of the tests of the BASIC test
   model (caserun.h) puts the halves of several passes into one session */
char *corpus_script_compile(const struct corpus_pass *p);
char *corpus_script_run(const struct corpus_pass *p);

/* stages configuration c: WORK/TAG/vol (CORPUS: with blitz2/, src/, out/,
   the job list and the files of the pass) and WORK/TAG/emu; 0 = ok, -1 =
   error (the message in log) */
int corpus_stage(const struct corpus_pass *p, size_t c, FILE *log);

/* a writable volume of a session */
struct corpus_volume {
    const char *volume;                 /* without colon */
    const char *host_dir;
    const char *cell;                   /* the directory of the first cell's
                                           run below host_dir (the volume
                                           BUNDLE: of a bundle, its c0), NULL:
                                           host_dir itself */
};

/* the file the hang check of hang_from=run arms on, below the directory
   vol (the first volume of the session), in its directory cell if not NULL
   (corpus_volume.cell): out/gate.run of the gate, else the first program's
   output out/NAME.run (with editrun=on its console out/NAME.con); NULL:
   none (no hang check) or out of memory */
char *corpus_hang_arm(const struct corpus_pass *p, const char *vol, const char *cell);

/* the emulator's inputs of a session of the pass (the machine, rtg=, uae=)
   as amiga_emulation_inputs_key gives them: the ROM, the hardfiles,
   Amiberry; 0 = none can be made */
unsigned long long corpus_emulator_key(const struct corpus_pass *p);

/* starts the emulator session of configuration c with the script text:
   the machine, the emulator changes (uae=, rtg=, floppy=, with port != 0
   the serial port as a TCP server on 127.0.0.1:port), the mounts, the time
   limit and the hang check of the pass, the work directory WORK/TAG/emu.
   vols NULL: CORPUS: is WORK/TAG/vol; else the nvols volumes given are the
   writable ones (the hang check of hang_from=run arms on the first program's
   output in the first of them, OUT/NAME.run, or in its directory cell).
   NULL = not started (err; amiga_emulation_no_slot() tells a missing slot) */
struct amiga_emulation *corpus_session_start(const struct corpus_pass *p, size_t c, const char *text,
                                             unsigned port, const struct corpus_volume *vols,
                                             size_t nvols, char *err, size_t errlen);

/* runs the pass: 0 = no differences, 1 = differences (report), 77 = the
   emulator or a file of the pass is missing (skipped), 2 = error; with
   expect= the bug-orig exit status above */
int corpus_run(struct corpus_pass *p, FILE *log);

#endif
