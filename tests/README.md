# Tests

Overview of the suites. The test presets say **where** the tests run;
labels select within them.

| Preset | Runs | Needs |
|---|---|---|
| `host` | tools with broken inputs, host tests, declaration and instruction comparisons, token lists (seconds) | native compiler; the CD for some |
| `emulator` | libraries against the CD's, corpus, TED A/B, compilers' extensions, CD corpus, RWE reference build | Amiberry (`AMIGA-EMULATION-SETUP.md`); the CD for most |
| `all` | both | both |
| `oracle` | `ab3_cli_ref` and its broken-input tests: our expect files of `tests/ab3` against AmiBlitz3 v3.14.0's own compiler (label `oracle`, not in `emulator`/`all`) | Amiberry; the fetched AmiBlitz3 files |
| `bug-orig` | reproduced original bugs (label `bug-orig`) and known port defects (label `bug-port`): they fail on purpose | Amiberry; the CD for most |

Setting up the emulator (ROMs, hardfiles, slot limit):
`AMIGA-EMULATION-SETUP.md`. The tool the emulator tests use, its sessions,
slots and snapshot mode: `tools/lib/amiga_emulation/README.md`.

Contents: [Original material and presets](#original-material-and-presets) ·
[host](#host-tests) · [emulator](#emulator-tests) · [bug-orig](#bug-orig-tests) ·
[bug-port](#bug-port-tests) · [oracle data](#oracle-data-of-the-ted-tests) ·
[Test statistics](#test-statistics) · [Coverage](#coverage)

## Original material and presets

Some tests need original material that is not in the repository (the CD,
the Aminet archive of the sources): once

```
cmake -P cmake/fetch-originals.cmake
```

(downloads about 40 MB, see `archive/README.md`; needs no build
directory). In an already configured build,
`cmake --build --preset amiga --target fetch-originals` works too. Without
these data the affected tests are skipped.

Building uses the preset `amiga` (cross compiler for AmigaOS; it also
builds the tools and host tests for it with the native compiler). The test
presets say **where** the tests run:

```
ctest --preset host       # on the host, without emulator (seconds)
ctest --preset emulator   # in Amiberry (label emulator)
ctest --preset all        # both
ctest --preset bug-orig   # reproduced original bugs, known port bugs: fail on purpose
ctest --preset oracle     # AmiBlitz3's own compiler checks the expectations of tests/ab3
```

The preset `oracle` (label `oracle`) holds the tests that run the original
tool to validate our expectations: `ab3_cli_ref` compiles the cases of
`tests/ab3` marked REF with AmiBlitz3 v3.14.0's own compiler (Amiberry
without the JIT: the compiler crashes in its exit path under the JIT when
the host is loaded, #976) and compares the program output with the same
expect files; `ab3_cli_ref_broken` and `ab3_cli_ref_broken_date` check it with a wrong expect file.
The compiler never changes, so the result changes only with the cases and
expectations: run `ctest --preset oracle` before merging a change of
`tests/ab3` (cases, `cli/`, expect files, `cli.cmake`); `covsel` selects the
tests then. `host`, `emulator` and `all` leave them out.

## host tests

The tools with deliberately broken inputs, the host tests of
the TED core (compiled natively, snitch), the declaration comparison (`declarations_*`: header,
entries, `!libfin` of every built library against the original of the
CD, same libraries in the same order; `tools/blitzlibcompare`; the
intended differences of fixed original bugs are listed in
`libs/GROUP/declarations-intended.txt`, each of them must occur) and the
instruction comparison (`asmcmp_*`: the routines of runerrlib, debuglib,
dhandlerlib, memacclib that remained in assembler against the originals of the CD,
with `objdump` of the toolchain; `tools/asmcmp`). Both comparisons are
skipped without the CD. The token list (`tokens_*`: `tools/blitztokens`
makes it from a compiler and its libraries the way blitz2 builds it for
the editor) must equal `tests/data/ted-tokens.txt` – with the CD's
compiler and libraries (skipped without the CD) and with the ones built
here; `tokens_broken` checks that a missing library is noticed.
`rwe_residents_check_broken`: the comparison of two residents
(`tools/blitzres/check.cmake`) notices other contents. The host side on its own:
`ctest --test-dir build/amiga/host`.
With `-DHOST_SANITIZE=ON` the host side is built with AddressSanitizer
and UBSan. With `-DHOST_COVERAGE=ON` it is built with gcov
(`--coverage -O0`) for the program code the host tests compile (the
sources below `programs/*/src/` and `shared/`; not the tools, the test
sources or the TED tools), and each host test `X` writes its `.gcda`
files below `build/<preset>/host/coverage/X/` (`GCOV_PREFIX`).

## emulator tests

Run in Amiberry; without Amiberry or without the data in `amiga-emulation/`
(`AMIGA-EMULATION-SETUP.md`) they are skipped.

### Running them

* **Parallelism:** the presets `emulator`, `all` and `bug-orig` run 4
  tests at once (`execution.jobs`; ctest starts the tests that took
  longest in the last run first, from `Testing/Temporary/CTestCostData.txt`).
  Each test uses up to `CORPUS_PARALLEL` emulators (default 3 under
  Linux, otherwise 1), as far as there are free slots (a runner that
  holds a slot does not wait for more, it runs its sessions one after
  another). `CORPUS_TIMEOUT` limits a session; reports are in
  `build/<preset>/tests/corpus/NAME.txt`.
* **Slots:** over everything (all runners, build directories and
  worktrees of the machine) at most `AMIGA_EMULATION_MAX` Amiberry
  sessions run at once (environment, 0 = no limit; without it the number
  in the file `max` of the slot directory, default 4; how to change it
  while tests run: `AMIGA-EMULATION-SETUP.md`). A further session waits
  for a free slot (`tools/lib/amiga_emulation/README.md`, "Slots: how runners wait"); the
  wait does not count into the session's timeout, and every emulator test
  has `EMULATION_QUEUE_WAIT` seconds (default 7200,
  `cmake/emulation-queue.cmake`) more in its ctest TIMEOUT for it.
* **Snapshot mode:** a session with a slot starts from a savestate of the
  booted system instead of booting (about 13 s less per session; the
  first session per slot and configuration boots and saves it).
  `AMIGA_EMULATION_SNAPSHOT=0` turns it off. The snapshots belong to the
  build directory and are kept across ctest runs; a session restores one
  only while its key still matches, otherwise it boots, saves anew and
  removes the stale snapshot. `amiga_run --purge-snapshots [BUILDDIR]`
  removes a build directory's snapshots by hand. Details:
  `tools/lib/amiga_emulation/README.md`, "Snapshot mode".
* **A halted CPU:** a session whose emulated CPU Amiberry has halted (a
  double fault, a jump to no memory: it never runs again) ends at once
  with the state `halt` instead of waiting for its time limit
  (`timeout`) (`tools/lib/amiga_emulation/README.md`, "Session states").
* **Logs:** the presets `emulator`, `all` and `bug-orig` set
  `AMIGA_EMULATION_LOG=1` and `AMIGA_EMULATION_TIMEOUT_STATE=1`: every
  session writes Amiberry's log into `amiberry.log` of its working
  directory (in the build directory), and a session that runs into its
  time limit or halts leaves `timeout.txt` there (how it ended, CPU
  registers, whether the CPU is halted, its threads) with the machine's
  state (`timeout.uss`, `timeout.uae`;
  `tools/lib/amiga_emulation/amiga_emulation.h`).

### Libraries

`libs_emulator`: the test program `libs/tests/libs_tests` (snitch, m68k)
loads every acidlib of this build and its original of the CD like the
compiler does and compares results, registers and memory (skipped
without the CD; selection:
`cmake ... -DFILTER="[mathstr]" -P libs/tests/emulator.cmake`).

`rwe_residents_cd`: the residents `blitzres` writes have the contents
(`blitzres --dump`: types, macros, constants without the unused node
bytes) of those blitz2 2.10 of the CD and blitz2-c save with Create
Resident of the same constants as Blitz source (`blitzres --source`,
one corpusrun pass with `run=off` and tedstub `resident=`;
`libs/userlibs/rwe/residents.cmake`; skipped without the CD).

### Extensions of blitz2

`blitz2_ID` (`tests/blitz2/`): the extensions of blitz2
(`docs/blitz2/extensions.md`), which have no reference to compare with:
blitz2 compiles and runs `ID.bas` (header `; @runerrs on|off`) with the
CD's and with the C libraries, and both results must equal `ID.expect`
(one corpusrun pass with `expect=` and `regression=on`, blitz2 itself
as the first configuration; skipped without the CD).

### Corpus

The corpus (`tests/corpus/`): `corpus` (68 test programs
without runtime error checking), `corpus_runerr` (131 with), `corpus_req`
(3 with blocking requesters, behind the lock `reqclosetest`),
`corpus_serial` / `corpus_serial_runerr` (a program that talks over the
emulated serial port to a peer on the host; the bytes it sends over the
serial port are compared as well). Per pass, three configurations
compile the same sources and run the programs: ref (original blitz2 2.10
= `BLITZ2/BLITZ2` of the CD, libraries of the CD), port (blitz2-c,
libraries of the CD: executable equal, output equal), clib (blitz2-c
with the C libraries: output equal). Skipped without the CD.

`corpus_cd`: all 724 sources of the CD compiled with blitz2-c (like bb2's
bb2corpus: `.xtra` of the sources, assigns to CD directories, directory
of the relative INCLUDEs) and compared with `tests/data/blitz2-cd.tsv`
(result, error line and message, size, hash with masked trap blocks). The
table is generated by the original blitz2 2.10:

```
cmake --build --preset amiga --target blitz2-cd-reference   # about 35 s
```

A row whose note begins with an issue number (`#969: …`) holds the
port's result for an intended deviation (`docs/libs/deviations.md`,
kind F) with the reference's hash in the note; regenerating the table
writes the reference's row there, put the port's row back.

### TED A/B

`tests/ted-ab/`: `ted_ab` plays the scripts
`tests/ted-ab/scripts/*.drv` with `teddrive` against TED, in four
combinations (R5: original TED + blitz2 2.10, C5: TED-C + 2.10, Cc:
TED-C + blitz2-c, Rc: original TED + blitz2-c), checks the expectations
of the scripts and compares the runs pairwise (result, requesters,
screen dumps, files). Needs the ref run of `corpus` (fixture, ctest
starts it as well). Selection: `-DTED_AB_FILTER=02-*,29-cmp-*`,
`-DTED_AB_COMBOS=R5,C5`, sessions at the same time
`-DTED_AB_PARALLEL=N` (default 5, as far as emulator slots are free);
report in `build/<preset>/tests/ted-ab/ted_ab.txt`, about 13 minutes.
Known bugs (`; @known REASON` in the script) are not run and are listed
as KNOWN in the report; an exact name in the filter runs them anyway.
`ted_ab_lint` checks the scripts without the emulator.

### Reference build of the RWE sources

`rwe_refbuild` (fixture `rwe_refbuild`, about 20 s; `cdrun` with
`source=` lines over the staging directory, `RWE_REFBUILD_PARALLEL`,
`RWE_REFBUILD_TIMEOUT`; table
`build/<preset>/libs/userlibs/rwe/refbuild.tsv`) compiles the source of
every `RWE_LIBS` entry and fails if one does not compile.
It does not start the emulator again while nothing changed: after a
successful run, `cdrun` (`cache=` of the pass) writes a record of the
digest of its inputs (the staging directory with the sources, the
residents and `LIBNUMS.BB`, the CD's `blitz2` and libraries, the pass
file, the `cdrun` executable with `amiga_emulation` and its templates)
and the hashes of its outputs to
`build/<preset>/libs/userlibs/rwe/refwork/inputs.sha256`; the next
run with the same digest and all outputs unchanged ends at once
("no emulator run"). Anything missing or different means a full run
(host test `cdrun_cache`). `CDRUN_FORCE=1 ctest …` runs it anyway, e.g.
after changing the ROM, the hardfiles or Amiberry, which the digest
does not cover.

`rwe_refbuild_NAME`: the result equals the CD binary byte for byte, for
every entry of stage A (not for those in `RWE_REFBUILD_NEWER`, whose
reference is a newer binary); `rwe_refbuild_selftest` takes the dry
entry `RWE_REFBUILD_SELFTEST` (REFERENCE `refbuild`) through the
declaration comparison. A test that needs the reference build has the
label `emulator` (ctest starts a fixture's setup even for a test
outside the label filter).

## bug-orig tests

Tests of original bugs the port reproduces (`BUG(orig)`).
They check the *correct* behaviour and therefore **fail** as long as the
bug is reproduced; the failure message names the bug. Label `bug-orig`:
`host`, `emulator` and `all` exclude it, only the preset `bug-orig`
runs these tests. Naming: ctest `bug_orig_ID` (ID with `-` as `_`),
snitch test case `ID`; the issue names the test. In the snitch programs they carry the hidden tag `[.bug-orig]`,
so a plain run of the program skips them too (`libs_tests "[bug-orig]"`
or the name runs them). Tests in the emulator also have the label
`emulator` and are skipped (77) without Amiberry. So far
(`libs/tests/bugorig.c`, in Amiberry, only the C build of the
libraries, no CD needed, one emulator run each, time limit
`LIBS_BUG_ORIG_TIMEOUT`, default 60 s):
`bug_orig_cliargs_parpath_cli` (ParPath$ of a CLI argument: lock 0 to
NameFromLock), `bug_orig_crunch_implode_overread` (Implode reads
behind its data: the data end at the end of the highest memory region,
Amiberry logs the accesses to the unmapped memory behind it,
`log_illegal_mem`; `emulator.cmake -DILLEGAL_WATCH=ON` evaluates the
log), `bug_orig_crunch_decrunch_corrupt` (Deplode, CrMDecrunch,
PPDecrunch with corrupt data write outside the destination; guard
bytes), `bug_orig_crunchlib_crm_minsecdist`,
`bug_orig_crunchlib_crm_overlap_4gb`, `bug_orig_crunchlib_crm2_readtab`,
`bug_orig_crunchlib_pp_readd1` (one bug of the decrunchers each, #115
to #118), `bug_orig_debuglib_findoff_flags` (#127).

### With a Blitz program

**bug-orig tests with a Blitz program** (`tests/bug-orig/`, runner:
the corpus runner, `corpusrun` with `expect=`; verdict in
`tests/corpus/runner/bugorig.h`): a program that triggers the bug is
compiled and run in Amiberry by the original blitz2 1.95 with the
libraries of the CD (ref) and by the port (port: blitz2-c with the
libraries of the CD, clib: blitz2-c with the C libraries), one emulator
session each (in parallel), skipped without the CD. What a
configuration shows (the *observation*) are sections: `[session]`
(`done`, `timeout` for a crash or hang, `halt` for a crash that halted
the emulated CPU), `[compile]` (`ok`,
`error LINE MESSAGE`, `none`), `[run]` (output of the program with
`rc=N`), `[blitz2]` (what the compiler or the wrapper writes besides
the log, with `rc=N`), `[log]` (the log of tedstub). The expectation is
the **correct** behaviour, given only for the sections that matter
(`[session]` not given: `done`); a line ending in `*` matches by
prefix, a line `...` any number of lines. Verdict per configuration:
FIXED (matches the expectation), REPRODUCED (shows what the original
shows: the normal state, the test fails), DIFFERS (neither: the port
does not reproduce the original). **A valid bug-orig test** is one
whose original does *not* match the expectation; if it does, the test
is BROKEN (the original does not have the bug, or the expectation
describes the bug) and fails with that message. The test passes only
when every configuration is FIXED. Exit status of `corpusrun`: 0 FIXED,
1 REPRODUCED, 3 BROKEN, 4 DIFFERS. `bug_orig_verdict` (host) checks the
verdict with inputs from memory (among them an expectation equal to the
buggy output); `bug_orig_selftest_broken` (label `emulator`, not
`bug-orig`) runs such a broken test in Amiberry and must come out
BROKEN. Reports: `build/<preset>/tests/bug-orig/ID.txt`, the
observation of each configuration in `.../ID/TAG/observation`.
Helpers (`tests/bug-orig/amiga/`): `inject` runs the compiler with
faults (no free signal, failing AllocMem of a size, also only after
SKIP of them, Read or Write error (a Read also of a file that
OpenFromLock opened from a Lock of the name), failing Examine, failing
OpenLibrary or OpenDevice of a name, failing CreateProc, an UnLock of a
lock that a file owns after OpenFromLock counted and not done
(`ownedunlock`), DisplayAlert answered at once, exec's Alert of the process recorded and not shown (`alertlog`:
e.g. the AN_SemCorrupt that exec raises for one ReleaseSemaphore too
many and would otherwise show later, at a random moment; a dead-end
alert stops the process, the session ends by its time limit),
recorded allocations and FreeMem of another size than allocated;
with `pad` 64 zeroed bytes behind every allocation, also a hunk of
LoadSeg, a FreeMem inside an allocation is reported and not done, and
one whose 64 bytes are no longer zero is reported (written behind); with
`guard` the 64 bytes get an address-dependent pattern instead, checked
at FreeMem and at the end: writes behind a block are reported; for the
compiled programs with `runwrap=@INJECT@|...`) and reports what the
command leaves behind (library opens minus closes of the process, its
current directory and whether it unlocked the start lock, with
`cacheflush=NAME` whether the task that starts a process NAME flushed
the caches after its last AllocMem, paths still in
use); it also gives a program input while it runs: gadget clicks as
synthetic IntuiMessages (`click=TITLE/TEXT`, optionally while the DOS
list is locked for writing; with `undobuf` it reports whether a click
changed the UndoBuffer pointers of the window's string gadgets) and
several RAWKEY events in one call of
the input handlers at controlled times (`keys=`, an input handler with
a clock of its own); `wbrun` starts the compiler
(`wrap=@WBRUN@|...`) or a program (`runwrap=@WBRUN@|...`) like the
Workbench and reports the current directory at the end, how often the
process opened minus closed dos.library and files left in use; with
`freeseg` it overwrites and frees the seglist right after the reply, as
a memory checker would (a Blitz program
needs the command WBStartup, as from the real Workbench; what it
prints is lost), `userlib.c` is a library for
`blitz2:userlibs` (function `BugOrigUserLib`, token in
`userlib.tokens`). tedstub also runs menu items
(`menu=ID|ITEM|ANSWER`) and logs the busy pointer after them;
`run=ID|SOURCE|CONSOLE|PANIC` runs a source with COMPILE/RUN as from
the editor (the program in blitz2's own process, the stop requester
open; its console is the file CONSOLE, the output goes into the log),
optionally with !PANIC! hit PANIC ticks after the program started, and
logs the requesters left open;
`strbuf=N` sets the string buffer of the default options (strings
longer than 10240 bytes), `macrobuf=N` their macro buffer, `objbuf=N`
their object buffer,
`max=LIB|N` the object maximum of the library number LIB (e.g. 160 =
bitmaplib: more BitMap objects);
`resident=ID|SOURCE|RESFILE` (up to 8) compiles SOURCE before the first
job with the start options, saves it as the resident RESFILE (Create
Resident) and enters RESFILE in the resident list of the default
options, so the test program is compiled with the residents. All as
`; @pass jobline=...`.

### Adding a test

*Adding a test* (no shared list, a glob finds the files): two files in
an area directory (`compiler/`, `libs/` ...), named after the bug ID of
the issue: `ID.bas` (the program as text; in its header comment
`; @pass KEY=VALUE` for extra lines of the pass file, see
`tests/corpus/runner/corpus.h`: `file=`, `dir=`, `remove=`, `wrap=`,
`runwrap=`, `runargs=` (arguments of the program, a CLI start),
`uae=KEY=VALUE` (a change of the emulator configuration), `jobline=`,
`timeout=`, `runtime_errors=on`, `serial=on` (the remote end of the
serial tests on the emulated serial port, `tests/corpus/runner/serial.h`:
canned replies, among them `N` with a byte count and `S`, an XOFF that
holds the program's writes), with `@USERLIB@`,
`@INJECT@`, `@WBRUN@`, `@CD_BLITZ2@`, `@TESTDATA@`; `; @configs port` restricts the
configurations judged; `; @rwe NAME...` for the RWE userlibs, below) and `ID.expect` (the correct behaviour; a line
beginning with `[` is a section header, so the program's output must
not begin with `[`). Test
data (sounds, pictures, modules) is generated, not checked in: a
description `NAME.spec` in an area directory (`tools/mktestdata`:
bytes, words, longs, texts, labels, IFF chunks with their lengths filled
in; `tools/lib/mktestdata/mktestdata.h`) becomes
`build/<preset>/tests/bug-orig/testdata/NAME`, passed to the program
with `; @pass file=DEST|@TESTDATA@/NAME` (the program reads
`CORPUS:DEST`); NAME begins with the ID of the test. A further Blitz
source (e.g. the source of a resident) is a text `NAME.src`, tokenised
like `ID.bas` to `testdata/NAME`. Run
`ctest --preset bug-orig -R bug_orig_ID`; the result must be
REPRODUCED. Look at the observation of ref to write the expectation,
and check the expectation if possible with a temporary fix in the port
(then FIXED, the test passes); name the test in the issue.

### A bug of an RWE userlib

*A bug of an RWE userlib* (`libs/userlibs/rwe/NAME.c`): the same, in
`tests/bug-orig/rwe/ID.bas` with the header line `; @rwe NAME...` (the
libraries the program uses, e.g. also one that NAME calls). The
"original" is the library's reference (`RWE_LIBS`: the CD's or
AmiBlitz3's binary, or the reference build `rwe_refbuild` - then the
test requires that fixture, ctest runs it first, about 20 s): ref is
blitz2 2.10 of the CD (1.95 never loads a library of
`blitz2:userlibs/DIR`, #2) with the CD's libraries and the reference
in `blitz2:userlibs/rwe/`; port (blitz2-c, the CD's libraries) and clib
(blitz2-c with the acidlibs/deflibs of the library's variant,
`variants/rwe-1999`) get the C build there (the pass file's
`file=DEST|SRC|TAG`: a file for one configuration). The source is
tokenised with a token list of blitz2-c, that variant and the named
libraries (`blitztokens`, `tests/bug-orig/tokens/ID.txt` in the build
directory). The libraries of one test must have the same variant
(rievallib has its own, `rwe-1999-eval`, whose polylib number the CD's
acidlibs give to polylib: its tests need a ref with other acidlibs,
not done yet).

### A bug on a 68000/68010 or Kickstart 1.x

*A bug that only shows on a 68000/68010 or Kickstart 1.x*: the header
line `; @machine NAME` (pass key `machine=`) runs every configuration
on that machine, `a500` (68000, Kickstart 1.3, Workbench 1.3.5) or
`a500-68010` (`AMIGA-EMULATION-SETUP.md`, "A500 machine"); compiler
and program run there, as on the A1200 (e.g. `libs/multi-68010-vbr`,
#42; `rwe/risortlib-finit-closelibrary-unchecked`, #824). On
Kickstart 1.x the session script is in AmigaDOS 1.3 syntax (the
redirection right behind the command name; no `$RC` and no `>>`:
`rcput` - `tests/corpus/amiga/rcput.c` - appends the `rc=N` of the
command before), tedstub (with the edstub frame) uses only Kickstart
1.2 functions, and `reqtrap` patches only AutoRequest there and calls
the program itself (LoadSeg, as the 1.3 CLI calls a command: only
machine code programs, not the BCPL commands of `C:`). reqtrap and
rcput are linked with libnix13 (libnix20, the toolchain's default,
does not start on Kickstart 1.x). The reference blitz2 1.95 is
assembled with `-kick1hunks` (HUNK_RELOC32: Kickstart 1.x's LoadSeg
knows no HUNK_DREL32). Not there on Kickstart 1.x: `gate=`
(reqclosetest, reqclose), `@INJECT@`, `@WBRUN@` (V36 functions).
blitz2-c starts there since #936 (linked with libnix13). A crash on Kickstart 1.3 ends in the requester
"Software error - task held", which reqtrap answers with Cancel: a
Guru that waits for the mouse, the session runs into its timeout.

### Port regression tests

**Port regression tests** (`tests/bug-orig/regression/ID.bas`, same
files and header lines): a bug of the port, not of the original, that
needs what only this runner gives (an RWE userlib, `uae=` such as a CD
image, `@INJECT@` ...). The expectation is what the reference does
(pass key `regression=on`): the test is BROKEN if the reference does
not match it, a configuration FIXED if it does and DIFFERS otherwise;
it passes when every configuration is FIXED. ctest `regression_ID`,
label `emulator` only (not `bug-orig`: it must pass in the presets
`emulator` and `all`); `regression_selftest_broken` (an expectation
the original does not show) must come out BROKEN. ID is named after
the issue of the port bug.

### bug-orig tests of TED

**bug-orig tests of TED** (`tests/bug-orig/ted/ID.drv`): a TED A/B
script (`tests/ted-ab`, directives in `runner/tedab.h`) whose
expectations are the correct behaviour. `tedab` in its bug-orig mode
(pass key `bugorig=1`) plays it against the original TED 1.1 under
blitz2 2.10 (R5, the reference) and TED-C (C5), each run in an
emulator session of its own (in parallel), and gives the verdict like
corpusrun (FIXED, REPRODUCED, BROKEN, DIFFERS; same exit status). The
observation of a run is its result (OK, `FAIL step N ...`, `TIMEOUT`
for a crash or hang, `HALT` for a crash that halted the emulated CPU) and the expectations it misses; REPRODUCED needs
the same observation and, when both runs are OK, the A/B comparison
SAME. Made for these tests: `; @file NAME "DATA"` (a file in `work/`,
`"..."` with `\n`, `\t`, `\xHH`, repeated lines append), `; @source F
NAME` (that file is loaded), `; @command ARGS` (instead of
`blitz2:blitz2 FILE`: e.g. `blitz2:ted @FILE@` for TED alone, or with
`TEDAB:tools/inject`/`TEDAB:tools/wbrun` in front, or `Execute` of a
script made with `@file`), `; @expect data NAME "DATA"`, `; @expect
[no]stdout "TEXT"`, `[no]log` (teddrive's log), `[no]screen DUMP
"TEXT"` (the decoded dump), `[no]file NAME "TEXT"`. ctest `bug_orig_ID`,
report `build/<preset>/tests/bug-orig/ID.txt`, the runs in
`.../ID/sNNN/vol/runs/ID/{R5,C5}`; `bug_orig_ted_lint` (host) checks
the scripts, `bug_orig_ted_selftest_broken` (label `emulator`) must come
out BROKEN. Adding a test: one file `ted/ID.drv`, found by a glob.

### Order of the bug-orig tests

**Order of the bug-orig tests:** ctest with several jobs runs the
tests that failed in its last run first, in declaration order and
without regard to `COST`, and the bug-orig tests fail on purpose. So
all of them (those of `libs/tests` too) are recorded with
`bug_orig_add_test` and declared at the end of
`tests/bug-orig/CMakeLists.txt`, sorted by their `COST` (session limit
times sessions), the largest first (`cmake/bug-orig-order.cmake`);
ctest `bug_orig_order` (host) checks the order, `bug_orig_order_broken`
the check with a wrong one.

**Order of the long tests of the all suite:** the longest tests
(`ted_ab`, the ab3 equivalence passes, `libs_emulator`, the corpus
passes, ...) get a `COST` (rounded seconds, `cmake/test-cost.cmake`), so
that a cold run, without `CTestCostData.txt`, starts them first and does
not end with one of them alone.

## bug-port tests

Tests of known defects of the port itself (not of the
original), each with an issue. The test checks the *correct* behaviour
and **fails** until the defect is fixed; the fix removes the label from
the test. Label `bug-port` (with `emulator`): `host`, `emulator` and
`all` exclude it like `bug-orig`; the preset `bug-orig` runs both
(`ctest --preset bug-orig -L bug-port`: only these; with no open
defect the label has no test, and ctest reports "No tests were found"
with success). `tests/bug-port/` (`bug_port_lsclear(... BUG ...)` sets
the label): `bug_port_963_ted` and `bug_port_963_ab3cli` (#963, fixed:
no label any more, they run in `emulator` and `all`): `lsclear`
(`amiga/lsclear.c`, no libc) dirties all free memory with `$AA` on the
`a500` machine (Kickstart 1.3), loads the program with `LoadSeg` and
checks that every byte between a hunk's stored block and its header
size is 0 (Kickstart 1.x clears only `HUNK_BSS`; the baserel programs
without startup code have their `.bss` in the data hunk, which
`cmake/baserel-nostart.ld` stores whole). The checker has tests of its own (label
`emulator`, they pass): `lsclear_selftest_a500` / `_a1200` (generated
load files with an unstored tail, a stored zero tail and a `HUNK_BSS`:
on Kickstart 1.3 only the first is FAIL, with counted bytes; all PASS
on Kickstart 3.2), `lsclear_selftest_wrong_*` (wrong expectations must
fail), `lsclear_ted_a1200` (control: `ted` passes on Kickstart 3.2).


## Oracle data of the TED tests

The oracle data of the TED tests (`tests/data/ted-oracle.sha256`: SHA-256
of detok, tokit2 and load/save for every source of the CD;
`tests/data/ted-tokens.txt`: the token list) are generated by the original
TED itself: `programs/ted/tests/oracle` loads the reference build of
`ted.s` as a library, under the original blitz2 1.95 in Amiberry (needs
the CD):

```
cmake --build --preset amiga --target ted-oracle     # about 40 s
cmake -DORACLE_LIMIT=5 build/amiga                   # trial run, tests/data stays
```

## Test statistics

Every test preset writes a JUnit file of its run into the build
directory: `build/amiga/junit-PRESET.xml` (`build/native/junit-native.xml`),
replaced by the next run of the preset. To keep several runs, name the
file on the command line (relative to the build directory; it overrides
the preset's):

```
ctest --preset bug-orig --output-junit junit-bug-orig-1.xml
ctest --preset bug-orig --output-junit junit-bug-orig-2.xml
```

The presets keep the whole output of passing tests as well (up to 1 MiB
per test, cut at the front if longer: `maxPassedTestOutputSize`,
`testOutputTruncation`), so it is in the file (`<system-out>`).

The runners of the emulator tests (`corpusrun`, `cdrun`, `tedab`, also in
the bug-orig mode) write one line per emulator session to their output,
after their report:

```
@stat v=1 test=audiolib-diskplay-endless config=clib machine=a1200 snapshot=restored session=45.16 wait=0.00 timed=45.15 limit=45 end=timeout verdict=REPRODUCED
```

test (the pass), config (corpus: ref/port/clib; TED: R5/C5/... in the
bug-orig mode, else the session sNNN; cdrun: the worker), machine, what
the snapshot mode did (cold, restored, first-save, fallback), the session
time without the wait for an emulator slot, the wait, the part counted
against the time limit, the limit, the end (done, timeout, halt,
exited, failed) and the verdict of the config (bug-orig FIXED/REPRODUCED/DIFFERS/
BROKEN, reference; corpus same/differs; TED A/B ok/fail). The format is
defined in `tools/lib/teststats/statline.h`. `teststats` reads one or
more JUnit files (each one a run):

```
build/amiga/host/tools/teststats/teststats [--tsv] [--factor X] [--floor SEC] build/amiga/junit-*.xml
```

It prints per test the runs and their durations (min/median/max), per
test and config the sessions (max session time, max time counted against
the limit, max slot wait, timeouts, verdicts), the flaky ones (ctest
result, verdict or end differs between runs), suggested time limits (max
timed × factor, default 3, at least the floor, default 30 s; a config
that ended by timeout in every session of every run is listed as hanging,
without a number) and a suggested ctest `COST` per test (the median
duration; a test's duration includes its waits for emulator slots). Broken input (malformed XML, a testcase without name or time,
a bad `@stat` line) is reported on stderr (exit status 1), the rest
still counts.

## Coverage

Which code each test executes, as one lcov tracefile per test (openspec
change `test-coverage`). It is made from a coverage run, not checked in:

```
cmake --preset amiga-cov && cmake --build --preset amiga-cov
ctest --preset coverage                                  # about 26 minutes
cmake --build build/amiga-cov --target coverage-map      # seconds
```

The configure preset `amiga-cov` (directory `build/amiga-cov/`) builds
`blitz2` and `blitz2-ab3` with function coverage (`BLITZ2_COVERAGE`) and
the host side with gcov (`HOST_COVERAGE`); the normal configuration and
its results stay as they are. The test preset `coverage` runs every test,
`all` and `bug-orig` together, in that build: the tests of reproduced
original bugs fail on purpose there as in `ctest --preset bug-orig` (291),
all others pass.

* **host tests:** a build with `-DHOST_COVERAGE=ON` (gcov, `-O0`) of the
  program code they compile (`programs/*/src/`, `shared/`: the TED core,
  the `.xtra` model, the tokenizer, ...; gcc's
  `-fprofile-filter-files`); the tools, the test sources and the TED
  tools are not instrumented. Each host test `X` leaves its `.gcda` files
  below `build/<preset>/host/coverage/X/` and is recorded as test
  `host/X`.
* **compilers in the emulator:** the coverage configuration builds
  `blitz2` and `blitz2-ab3` instrumented; each emulator test leaves the
  coverage files of the compilers it ran as
  `coverage/<test>/<program>.cov` in the build directory (format:
  `shared/cov/covfmt.h`). Line ranges of their functions come from the
  objects if they are compiled with `-g` (which leaves the code
  unchanged); without them a change anywhere in a source file counts for
  all its functions.

Before a run clear `build/amiga-cov/host/coverage/` (gcov adds to data
that is there). A rebuild of the host side after the run makes gcov refuse
the data of the rebuilt objects (a stamp mismatch, and `coverage-map`
fails): run the host tests again (`ctest --preset coverage -R
'^(host|host_emulator)$'`). After the run

```
cmake --build build/amiga-cov --target coverage-map
```

runs `cmake/coverage-map.cmake`: gcov per host test, `nm -A` and
`objdump --dwarf=decodedline` of each measured program's objects, and
`tools/covmap` over all of it. The results, in `build/<preset>/coverage/`:

* `lcov/<test>.info`: the lcov tracefile (format of lcov 2.x) of each
  test (`lcov/host/X.info` for host test `X`), its name as `TN:` (in
  lcov's characters: `[A-Za-z0-9_]`, every other one as `_`). Per source
  file every function of the measured code the test ran with its first
  and last line (`FN:`) and its count (`FNDA:`); the host tests also with
  line and branch counts (`DA:`, `BRDA:`). The compilers' coverage files
  give each function a count of 1 if it was entered, else 0, and no line
  counts: the one `DA:` of such a function is on its first line and holds
  the function's count (its entry, so that `genhtml` shows the file); a
  function whose lines are not known has `FN:1,<name>` without a last
  line and no `DA:`. Details: `tools/lib/covmap/covmap.h`.
* `lcov/all.info`: all tests in one tracefile, for `genhtml`
  (`genhtml -o <dir> build/amiga-cov/coverage/lcov/all.info`, lcov 2.0
  or later; lcov is not needed by the build), editors and coverage
  services; `--branch-coverage` adds the host tests' branches (checked
  with lcov 2.6). The report lists the compilers' files with their
  functions; their "lines" are the functions' entry lines, so a file's
  line rate there is the rate of its functions. With
  `--branch-coverage`, lcov warns `inconsistent: no branch data for
  <test> in <file>` for the files both the host tests and the compilers
  measure (the compilers give no branches); it is a diagnostic only
  (`--ignore-errors inconsistent` silences it). A file whose function
  lines are not known (`glue_direct.c`) is not in the report.
* `commit`: the commit the run was made from.

The inputs handed to covmap stay in `build/<preset>/coverage/map-inputs/`.

While working on a change, the tracefiles choose the tests it can affect:

```
cmake -DBUILD=build/amiga -DCOVERAGE=build/amiga-cov/coverage \
      [-DBASE=<commit>] -P cmake/coverage-select.cmake
ctest --preset all --tests-from-file coverage/selected.txt
ctest --preset bug-orig --tests-from-file coverage/selected.txt
```

(ctest reads a relative `--tests-from-file` in the build directory.)

`cmake/coverage-select.cmake` runs `git diff -U0` of the working tree
against the coverage's commit (and lists the files git does not track),
`ninja -t deps` and `ctest --show-only=json-v1` of the build and its
host project, and hands it all to `tools/covsel`. It prints why each
test is selected and writes `build/amiga/coverage/selected.txt`. The
rules for what a change of a file selects are in
`tools/covsel/rules.txt`, the first matching pattern counts:

* a changed line of a measured source (`programs/blitz2/src/*.c`): the
  tests that executed the function around it; a line outside every
  function, or a source without line ranges, the tests of all its
  functions;
* a header of the compilers: each source whose object depends on it
  counts as changed as a whole, by its own rule (a measured source: the
  tests of all its functions; glue, which is not measured: all tests);
* a test source or expect file: the tests of the suites that read it (by
  name or label); a host test source: the host tests; the Amiga helper
  programs of the suites (`tests/*/amiga/`), which several suites use:
  all tests;
* what the emulator tests run on the host: the runners
  (`tests/*/runner/`), the emulator sessions
  (`tools/lib/amiga_emulation/`), `amiga_run`, the `@stat` line
  (`tools/lib/teststats/`) and `outmatch`: every emulator test and the
  host tests;
* the tools only the host tests and the coverage scripts use (`covmap`,
  `covsel`, `teststats`): the host tests; every other tool makes or
  checks results of the build (`biglib`, `blitzlibcheck`, `hunkcheck`,
  `xtra`, `ab3tok`, ...): all tests;
* `libs/`: the emulator tests and the checks of the libraries;
* documentation: no test;
* the build (CMake files, presets), TED, shared code, the test data, and
  every file no rule knows: all tests.

Tests without a tracefile that names an executed function are always
selected: new ones, those that run no measured program, and those whose
compiler never reaches its normal end (the coverage file is written
there: a compiler that hangs or crashes under a bug-orig test leaves
none). With the map of the coverage run below that is a fixed part of
162 of the 495 tests in every selection; a change of one `eval.c`
function entered by two tests selects 164. If the coverage's commit is
not an ancestor of `BASE` (default: the coverage's commit), the coverage
is stale and all tests are selected. The selection is for the work in
between: the full runs (`ctest --preset all`, `ctest --preset bug-orig`)
stay the acceptance of every program change.

### Compiler functions no test reaches

The coverage run of commit `e2db70e` (all 495 tests): of the 1132
functions of `blitz2` and `blitz2-ab3` (their sources in
`programs/blitz2/src/` and the shared code they link), 888 are entered by
an emulator test, 53 only by host tests (the shared code of the `.xtra`
files and the tokenizer), and 191
by no test. Of these, 9 are not measured (the assembler entry points of
`glue.c` and `glue_direct.c`, and `shared/cov/cov.c` itself); the other
182:

* 103 of the 108 in `parse.c` are compile error entry points (one
  function per message, `alconsterr` "Constant already defined",
  `mismatcherr` "Type Mismatch", ...): the tests provoke few compile
  errors. Four more such functions are in `compile.c`, `main.c` and
  `tokens1.c`;
* the editor side of the compiler: 23 of the 27 functions of `data.c`
  (the menu actions: options, Create Resident, the list and type
  requesters) and 10 in `main.c` (`ed_*` screen, gadget and tokenising
  calls, `flash`, `menu8`): the tests compile from the CLI or through
  `tedstub`;
* 11 in `eval.c` (code emitters of rarely used operators such as
  `doeorb`, `doasrb`, `dolslb`, and `pevalu`/`bakpevalu`), 5 in
  `ext_bb2.c` (the BB2 side of extension points whose syntax no test
  uses, e.g. `ext_dcbescape`), the rest single functions in `asmlibs.c`
  (`fetchpees*`), `stmts.c`, `tokens1.c`, `tokens2.c`, `compile.c`,
  `main.c` and `libcall.c`.

The list (file, function, lines) is made from the tracefiles of a
coverage run: the functions with `FNDA:0` in every tracefile.

