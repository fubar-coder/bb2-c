# Blitz Basic 2 in C

The compiler **blitz2** (Blitz Basic 2 v2.10: the source of 1.95 with the
changes of the CD's compiler, `docs/v2.10/`), the editor **ted** (TED 1.1)
and the libraries blitz2 reads at startup (**acidlibs**, **deflibs**), as
a C translation of the original sources – built for AmigaOS with
m68k-amigaos-gcc.

This repository contains the build, the reference builds, the tests
(host, corpus, TED A/B and library tests in the emulator) and the notes on
the translation (analysis of the originals, deviations) in `docs/`. It
replaces the former working repository `bb2` (state of the takeover: bb2
`7b5df74`); only bb2's analysis tools (Python) were not taken over.

## Requirements

* CMake ≥ 3.28 and Ninja
* m68k-amigaos-gcc (bebbo/AmigaDev, tested with gcc 16.2-rc21) in `PATH`,
  otherwise `-DAMIGA_TOOLCHAIN_PREFIX=…` or the environment variable of the
  same name; gcc ≥ 15 because of `#embed`, 16.2-rc21 or later because of
  `__saveds` (AmigaPorts/gcc#141; configure checks it,
  `cmake/check-saveds.cmake`)
* a C compiler for the host (for the tools in `tools/`)

## Building

```
cmake --preset amiga
cmake --build --preset amiga
cmake --install build/amiga        # optional: build/amiga/install/blitz2/
```

Result in `build/amiga/blitz2/` – the contents of the `blitz2:` directory:

| File | Contents |
|---|---|
| `blitz2` | the compiler |
| `ted` | the editor (blitz2 loads it as `blitz2:ted`) |
| `acidlibs` | the 83 command libraries |
| `deflibs` | the 39 declaration libraries of the AmigaOS calls |

Preset `amiga-debug`: the same code (`-Os`) with symbols, without strip.

**Variants** of the libraries: a directory `libs/variants/NAME/` with a
`libnums.h` (the library numbers) is a variant. The default variant
(`-DBLITZ_DEFAULT_VARIANT=NAME`, default `bb2-2.10`: the numbers of the
CD's `libnums.bb`) is built to `build/amiga/blitz2/acidlibs` and
`deflibs` and checked against the CD; every other one to
`build/amiga/variants/NAME/acidlibs` and `deflibs` (targets
`NAME-acidlibs`, `NAME-deflibs`). A variant's `libnums.h` can include the
one of another (`../bb2-2.10/libnums.h`) and change single numbers
(`#undef`/`#define LIB_xxx`). Every number in the libraries comes from
`libnums.h` – header, `!libs` references, the calls of other libraries
(ALibJsr) – so a changed number moves all of them, and the tokens of a
source written for the variant come from its files (`blitztokens`,
`teddetok --blitz2 … --lib …`). Test variants are in
`libs/tests/variants/` (ctest `variant_*`).

**Userlibs** of a variant: `libs/userlibs/DIR/NAME.c` →
`build/amiga/variants/VARIANT/userlibs/DIR/NAME.obj` (for
`blitz2:userlibs/DIR/`). `libs/userlibs/rwe/`: the libraries of Red When
Excited's sources (`docs/rwelibs/README.md`), variant `rwe-1999`; the
list `RWE_LIBS` there names per library the Blitz source and its CD
binary. They are checked like the acidlibs: declarations equal to the
binary's (ctest `rwe_declarations_NAME`, `rwe_numbers`), behaviour
compared with it in Amiberry (`libs/tests/rwe/NAME.c`, test cases
`rwe NAME` of `libs_emulator`). A library whose number the acidlibs
have is built in a variant of its own (`RWE_OWN_VARIANTS`):
`rwe-1999-eval` is `rwe-1999` with polylib moved from 121 to 87
(`#renderlib`) for RIEvalLib (121,
`variants/rwe-1999-eval/userlibs/rwe/rievallib.obj`, used with the RWE
userlibs of `rwe-1999`, whose numbers are the same; ctest
`variant_rwe_1999_eval`, `rwe_numbers_rwe-1999-eval`); `rwe-1999-ahi`
has `rwe-1999`'s numbers and risoundlib2 instead of risoundlib (both 45:
`variants/rwe-1999-ahi/userlibs/rwe/risoundlib2.obj`, used with the RWE
userlibs of `rwe-1999` but `risoundlib.obj`; ctest
`variant_rwe_1999_ahi`, `rwe_numbers_rwe-1999-ahi`). Target
`rwelibsrc`: those Blitz sources as TED shows them, in
`build/amiga/rwelibsrc/` (like `bblibsrc`), with the tokens of the
library files of their variant built here (blitz2, the variant's
acidlibs and deflibs, all RWE userlibs; needs the Aminet archive, not
the CD). The **reference build** of
those sources (`libs/userlibs/rwe/refbuild.cmake`,
`docs/rwelibs/README.md` §3.1): the build stages them (target
`rwe_refstage`: `build/amiga/libs/userlibs/rwe/refstage/` with the CD's
`EXTRAS/BLITZLIBS/`, the residents `lvo.res`, `custom.res`,
`datatypes.res`, `AHI.res` that `tools/blitzres` writes from the toolchain's NDK
(target `rwe_residents`), a `LIBNUMS.BB` with the numbers of the
variant's `libnums.h`, the sources with their `INCDIR` lines neutralised
by `tools/bblines`), ctest `rwe_refbuild` compiles them with the CD's
blitz2 2.10 in Amiberry to `build/amiga/libs/userlibs/rwe/ref/NAME.obj`.
An `RWE_LIBS` entry with REFERENCE `refbuild` is compared with that
file.

## IDE (CLion and others)

The build uses two compilers: the m68k-amigaos-gcc for everything that
runs on the Amiga, the native compiler for the host side (`host/`: the
tools the build uses and the host tests). One CMake build has one
compiler per language, so the m68k build runs the host side as
`ExternalProject` (`build/<preset>/host`) – an IDE does not see its
targets. Hence two profiles from `CMakePresets.json`:

* **`amiga`** (or `amiga-debug`): the real build. Editing, navigation and
  building of blitz2, ted and the libraries; the m68k programs do not run
  on the host – their tests run through CTest (the emulator tests in
  Amiberry). Without a preset the m68k toolchain is the default too.
* **`native`** (`BLITZ2_HOST=ON`, `build/native`, Debug): only the host
  side as a project of its own, native compiler – every tool and host
  test is a target of its own, to run and debug in the IDE; CTest lists
  its 58 tests one by one (`ctest --preset native`).

CLion reads the presets as CMake profiles (Settings → Build, Execution,
Deployment → CMake: enable `amiga` and `native`; the profile CLion adds
itself, `Debug` in `cmake-build-debug`, is not needed). The toolchain is
found through `PATH`, else in `/opt/amiga` or `/opt/m68k-amigaos`, else
set `AMIGA_TOOLCHAIN_PREFIX` (environment or cache). Tests are not
projects of their own: they are targets of the same CMake project,
registered with `add_test`, and run by CTest – in CLion through a CTest
run configuration (labels as arguments, e.g. `-LE emulator`). The host
tests use snitch (a Catch2-like framework), which CLion does not know
as a test framework; they run as CTest tests and as plain programs.
`.clangd` (generated) is for clangd-based editors; it holds the m68k
options clangd does not know.

## Checks in the build

The build checks its results itself; if a check fails, the result counts
as not built, and nothing that depends on it is built (the next run checks
again):

* every library file (`tools/blitzlibcheck`, in the same step as link and
  strip): one code hunk, relocs only into it, library header, token table
  readable up to the end, no reference from the normal code into the error
  check area (without the debugger it would point behind the end of the
  code; the originals have such references, the port has them fixed).
  `acidlibs`/`deflibs`
  are made only from checked libraries.
* `acidlibs`, `deflibs` (also of every variant), in the same step as the
  join, and every userlib in its build step (against the acidlibs and
  deflibs of its variant): no library number twice (`biglib check`; deflibs together with
  the acidlibs loaded with it) – two libraries with one number would have
  the same tokens.
* `blitz2` after every link (POST_BUILD): `programs/blitz2/checklink.cmake`
  on the link map - no libnix20 member and no base of a library that
  Kickstart 1.3 lacks (`UtilityBase`): blitz2 starts on Kickstart 1.x like
  the original (#936; ctest `blitz2_checklink_broken_*` with the map of
  the earlier link).
* reference build of blitz2: the original
  `archive/sources/blitz2-1.95/blitz2.s` with vasm to
  `build/amiga/programs/blitz2/ref/` (`blitz2`, listing, `blitz2.map`,
  `blitz2.labels`), in the same step `vasmlisting verify`.
* reference build of TED: the original `archive/sources/ted-1.1/ted.s`
  with vasm (like Devpac) to `build/amiga/programs/ted/ref/` (`ted`,
  listing, `ted.map`, `ted.labels`); the same step checks that listing and
  binary match (`vasmlisting verify`). The reference for the port is this
  build, not the TED binary of the CD (a different build).
* `ted` after every link (POST_BUILD): hunk layout (`tools/hunkcheck`:
  offset 0 `bsr.w`, offset 4 `outside`) and `programs/ted/checklink.cmake`
  (no libgcc/libnix helper functions that would need `UtilityBase`) and
  `tools/checklabels`: every code label of `ted.s` exactly once as
  `ted.s:NNN label` in `programs/ted/src/` (completeness of the
  translation; a changed comment line rebuilds the object, so the check
  runs then as well).
* the residents of the RWE reference build (`tools/blitzres`, in the
  same step as the write): read back, the file holds exactly the
  constants written. Its staging: `tools/bblines` fails on an `INCDIR`
  line with a further statement, the `LIBNUMS.BB` step on a number of
  `libnums.h` the CD's file has with another value (ctest
  `rwe_refbuild_libnums_broken`).
* `ted.sym` (the same link with symbols) after every link:
  `tools/checkdata` compares the data of the C build with the reference
  build of `ted.s` (`programs/ted/checkdata.tsv`: 534 entries – fonts,
  mouse pointers, option block, texts, screen, windows, gadgets, menus,
  requesters; relocated longwords of the reference skipped). `ted` is
  linked only after that.

## Tests

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
ctest --preset bug-orig   # reproduced original bugs: fail on purpose
```

* **host:** the tools with deliberately broken inputs, the host tests of
  the TED core (compiled natively, snitch), the declaration comparison (`declarations_*`: header,
  entries, `!libfin` of every built library against the original of the
  CD, same libraries in the same order; `tools/blitzlibcompare`) and the
  instruction comparison (`asmcmp_*`: the routines of runerrlib, debuglib,
  dhandlerlib, memacclib that remained in assembler against the originals of the CD,
  with `objdump` of the toolchain; `tools/asmcmp`). Both comparisons are
  skipped without the CD. The token list (`tokens_*`: `tools/blitztokens`
  makes it from a compiler and its libraries the way blitz2 builds it for
  the editor) must equal `tests/data/ted-tokens.txt` – with the CD's
  compiler and libraries (skipped without the CD) and with the ones built
  here; `tokens_broken` checks that a missing library is noticed.
  `rwe_resident_NAME`: the residents `blitzres` writes have the contents
  (`blitzres --dump`: types, macros, constants without the unused node
  bytes) of those blitz2 2.10's Create Resident made in the analysis
  (`RWE_ANALYSIS_DIR`, default `.cache/rwelibs-analysis`, not in the
  repository: skipped without). The host side on its own:
  `ctest --test-dir build/amiga/host`.
  With `-DHOST_SANITIZE=ON` the host side is built with AddressSanitizer
  and UBSan.
* **emulator:** runs in Amiberry; without Amiberry or without the data in
  `amiga-emulation/` they are skipped. `libs_emulator`: the test program
  `libs/tests/libs_tests` (snitch, m68k) loads every acidlib of this build
  and its original of the CD like the compiler does and compares results,
  registers and memory (skipped without the CD; selection:
  `cmake ... -DFILTER="[mathstr]" -P libs/tests/emulator.cmake`). Among
  them the corpus
  (`tests/corpus/`): `corpus` (68 test programs without runtime error
  checking), `corpus_runerr` (131 with), `corpus_req` (3 with blocking
  requesters, behind the lock `reqclosetest`),
  `corpus_serial` / `corpus_serial_runerr` (a program that talks over the
  emulated serial port to a peer on the host; the bytes it sends over the
  serial port are compared as well). Per
  pass, three configurations compile the same sources and run the
  programs: ref (original blitz2 2.10 = `BLITZ2/BLITZ2` of the CD,
  libraries of the CD), port
  (blitz2-c, libraries of the CD: executable equal, output equal), clib
  (blitz2-c with the C libraries: output equal). Skipped without the CD.
  `CORPUS_PARALLEL` (default 3 under Linux, otherwise 1) limits the
  concurrent emulators, `CORPUS_TIMEOUT` a session; reports in
  `build/<preset>/tests/corpus/NAME.txt`. The presets `emulator`, `all`
  and `bug-orig` run 4 tests at once (`execution.jobs`; ctest starts the
  tests that took longest in the last run first, from
  `Testing/Temporary/CTestCostData.txt`); each one uses up to
  `CORPUS_PARALLEL` emulators, as far as there are free slots (a runner
  that holds a slot does not wait for more, it runs its sessions one
  after another). Over everything (all runners, build
  directories and worktrees of the machine) at most
  `AMIGA_EMULATION_MAX` Amiberry sessions run at once (environment,
  0 = no limit; without it the number in the file `max` of the slot
  directory, e.g. `echo 6 > $XDG_RUNTIME_DIR/amiga-emulation-slots/max`,
  read again while a session waits, so it can be changed while tests
  run; missing or not a number from 1 to 64: 4; see
  `amiga-emulation/README.md`): a further session waits for a free slot
  (`tools/lib/amiga_emulation`; the wait does not count into the
  session's timeout, and every emulator test has `EMULATION_QUEUE_WAIT`
  seconds (default 7200, `cmake/emulation-queue.cmake`) more in its
  ctest TIMEOUT for it). A session with a slot starts from a savestate
  of the booted system instead of booting (snapshot mode, about 13 s
  less per session; the first session per slot and configuration boots
  and saves it); `AMIGA_EMULATION_SNAPSHOT=0` turns it off
  (`amiga-emulation/README.md`, "Snapshot mode"). The snapshots belong
  to the build directory and are kept across ctest runs: a session
  restores one only while its key, a hash of what the booted state is
  made from (Kickstart ROM, hardfiles and other media by real path, size
  and time; the configuration without the session's paths; the machine;
  Amiberry; the waiter), still matches; otherwise it boots, saves anew and
  removes the stale snapshot. `amiga_run --purge-snapshots [BUILDDIR]`
  removes a build directory's snapshots by hand. `corpus_cd`: all 724
  sources of the CD compiled with blitz2-c (like bb2's bb2corpus: `.xtra` of the sources,
  assigns to CD directories, directory of the relative INCLUDEs) and
  compared with `tests/data/blitz2-cd.tsv` (result, error line and
  message, size, hash with masked trap blocks). The table is generated by
  the original blitz2 2.10:

  ```
  cmake --build --preset amiga --target blitz2-cd-reference   # about 35 s
  ```

  TED A/B (`tests/ted-ab/`): `ted_ab` plays the scripts
  `tests/ted-ab/scripts/*.drv` with `teddrive` against TED, in four
  combinations (R5: original TED + blitz2 2.10, C5: TED-C + 2.10, Cc:
  TED-C + blitz2-c, Rc: original TED + blitz2-c), checks the expectations
  of the scripts and compares the runs pairwise (result, requesters,
  screen dumps, files). Needs the ref run of `corpus` (fixture, ctest
  starts it as well). Selection: `-DTED_AB_FILTER=02-*,29-cmp-*`,
  `-DTED_AB_COMBOS=R5,C5`; report in `build/<preset>/tests/ted-ab/
  ted_ab.txt`, about 13 minutes. Known bugs (`; @known REASON` in the
  script) are not run and are listed as KNOWN in the report; an exact
  name in the filter runs them anyway. `ted_ab_lint` checks the scripts
  without the emulator.
  The reference build of the RWE sources: `rwe_refbuild` (fixture
  `rwe_refbuild`, about 20 s; `cdrun` with `source=` lines over the
  staging directory, `RWE_REFBUILD_PARALLEL`, `RWE_REFBUILD_TIMEOUT`;
  table `build/<preset>/libs/userlibs/rwe/refbuild.tsv`) compiles the
  source of every `RWE_LIBS` entry and fails if one does not compile.
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
* **bug-orig:** tests of original bugs the port reproduces (`BUG(orig)`).
  They check the *correct* behaviour and therefore **fail** as long as the
  bug is reproduced; the failure message names the bug. Label `bug-orig`:
  `host`, `emulator` and `all` exclude it, only the preset `bug-orig`
  runs these tests. Naming: ctest `bug_orig_ID` (ID with `-` as `_`),
  snitch test case `ID`; the issue names the test. In the snitch programs they carry the hidden tag `[.bug-orig]`,
  so a plain run of the program skips them too (`libs_tests "[bug-orig]"`
  or the name runs them). Tests in the emulator also have the label
  `emulator` and are skipped (77) without Amiberry. So far
  (`libs/tests/bugorig.c`, in Amiberry, only the C build of the
  libraries, no CD needed, one emulator run each):
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

  **bug-orig tests with a Blitz program** (`tests/bug-orig/`, runner:
  the corpus runner, `corpusrun` with `expect=`; verdict in
  `tests/corpus/runner/bugorig.h`): a program that triggers the bug is
  compiled and run in Amiberry by the original blitz2 1.95 with the
  libraries of the CD (ref) and by the port (port: blitz2-c with the
  libraries of the CD, clib: blitz2-c with the C libraries), one emulator
  session each (in parallel), skipped without the CD. What a
  configuration shows (the *observation*) are sections: `[session]`
  (`done`, `timeout` for a crash or hang), `[compile]` (`ok`,
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

  *A bug that only shows on a 68000/68010 or Kickstart 1.x*: the header
  line `; @machine NAME` (pass key `machine=`) runs every configuration
  on that machine, `a500` (68000, Kickstart 1.3, Workbench 1.3.5) or
  `a500-68010` (`amiga-emulation/README.md`, "A500 machine"); compiler
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

  **bug-orig tests of TED** (`tests/bug-orig/ted/ID.drv`): a TED A/B
  script (`tests/ted-ab`, directives in `runner/tedab.h`) whose
  expectations are the correct behaviour. `tedab` in its bug-orig mode
  (pass key `bugorig=1`) plays it against the original TED 1.1 under
  blitz2 2.10 (R5, the reference) and TED-C (C5), each run in an
  emulator session of its own (in parallel), and gives the verdict like
  corpusrun (FIXED, REPRODUCED, BROKEN, DIFFERS; same exit status). The
  observation of a run is its result (OK, `FAIL step N ...`, `TIMEOUT`
  for a crash or hang) and the expectations it misses; REPRODUCED needs
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

### Test statistics

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
against the time limit, the limit, the end (done, timeout, exited,
failed) and the verdict of the config (bug-orig FIXED/REPRODUCED/DIFFERS/
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

## Readable library sources

The Blitz sources of the libraries (`bblibsrc.lha` in the Aminet archive,
unpacked to `archive/aminet/amibl2src/bblibsrc/`) are tokenized. The build
stores them as text, the way TED shows them – **line N of the file =
line N in TED**; line references like "bblibsrc/windowslib line 1277" in
the issues refer to this numbering:

```
cmake -P cmake/fetch-originals.cmake          # once (see Tests)
cmake --build --preset amiga --target bblibsrc
build/amiga/bblibsrc/NAME                     # e.g. windowslib, palettelib.aga
```

Same name as in the archive (without extension where the archive has
none); omitted are `NAME.xtra` (TED's editor state) and the data/object
files `*.bin`, `*.inc`, `*.obj`. The target belongs to `ALL` as soon as
the archive is present; without the archive it only reports that it is
skipped. The files are byte-identical to bb2 `libs-c/bb2src/NAME.bb`
(references `windowslib.bb:NNN` thus mean the same line); bb2 had only 82
of the 102. Several versions of the same library:
`palettelib`/`palettelib.aga`, `displaylib`/`displaylibfuk`/`olddisplaylib`,
`gtlib`/`oldgtlib`, `switchlib`/`switchlib.ok`,
`wbstartuplib`/`wbstartuplibnew`, `2dlib`/`sis2dlib.bb`.

The tool behind it is `teddetok` (`programs/ted/tools/teddetok`, on the C
core of TED like `tedtok`): loads a source like TED (`fileopen2`) and
writes every line detokenized. A token TED does not know (TED shows
`?????`) is an error, the file is then not written; a line longer than
TED's line buffer is split by TED when loading – `teddetok` reports that
and counts like TED.

```
build/amiga/host/ted-tools/teddetok/teddetok tests/data/ted-tokens.txt IN OUT
build/amiga/host/ted-tools/teddetok/teddetok tests/data/ted-tokens.txt --check IN...
build/amiga/host/ted-tools/teddetok/teddetok --blitz2 BLITZ2 [--lib LIB]... IN OUT
```

The token list is either a list file (`tests/data/ted-tokens.txt`, made
by the original TED) or it is made from the compiler and its libraries
(`--blitz2` and `--lib` in the compiler's order: acidlibs, deflibs, then
userlibs; library or BigLib files) as blitz2 builds it for TED – for
sources written with other libraries. `tedtok` takes the same forms.
`tools/blitztokens` writes such a list as a list file
(`blitztokens BLITZ2 LIB... > tokens.txt`), for both tools.

ctest `teddetok_*` checks it with small inputs (also broken ones: unknown
token, token byte at the end of the file, a compiler that is no load file)
and `teddetok_bblibsrc` all sources of `bblibsrc/` (skipped without the
archive); the `*_libs` tests do the same with the list made from the CD's
compiler and libraries (`tedtok_libs`: the same file as with the list
file).

Some of RWE's library sources (`rwelibsrc/`, 1997) are saved in another
format, `BB22` (`docs/rwelibs/README.md` §4.1: a header with the cursor,
a prefix before each line). `tools/bb22` converts them into TED's format,
for teddetok and the compiler; it refuses whatever it does not know:

```
build/amiga/host/tools/bb22/bb22 IN OUT
build/amiga/host/tools/bb22/bb22 --info IN
```

ctest `bb22` checks it with files built in memory (also broken ones),
`bb22_rwelibsrc` converts the 9 `BB22` files of `rwelibsrc/` and reads
them with teddetok (skipped without the archive or the CD). The
userlibs build converts a `BB22` source of `RWE_LIBS` the same way
before teddetok (target `rwelibsrc`) and before the staging of the
reference build (`build/amiga/libs/userlibs/rwe/ted/SOURCE`).

The companion settings of a source – the Blitz 2 `.xtra`, the cursor in
a `BB22` header, AmiBlitz3's "Embedded .xtra Header" of `.ab3` sources –
have one structure in `shared/xtra/xtra.h` (`docs/formats.md` §1.4, §7),
shared code that blitz2 and TED are meant to use later. `tools/xtra`
shows, checks and converts them:

```
build/amiga/host/tools/xtra/xtra dump FILE.xtra [--bb22 SRC]
build/amiga/host/tools/xtra/xtra dump --ab3 SRC.ab3
build/amiga/host/tools/xtra/xtra dump --source SRC
build/amiga/host/tools/xtra/xtra check [--bb22|--ab3] [--exact] FILE...
build/amiga/host/tools/xtra/xtra convert --to-ab3 IN.xtra OUT [--bb22 SRC | --ab3 SRC]
build/amiga/host/tools/xtra/xtra convert --to-xtra IN.ab3 OUT.xtra
```

ctest `xtra_blitz2`, `xtra_bb22`, `xtra_ab3` check the code with inputs
in memory (also broken ones), `xtra_tool` the tool on files,
`xtra_originals` all 575 `.xtra` of the CD and those of `rwelibsrc/`
(read and written again byte for byte) and the 9 `BB22` sources with
their `.xtra`, `xtra_ab3_examples` five AmiBlitz3 sources from GitHub
(`fetch-originals`; both skipped without the material).

## Emulator

`tools/lib/amiga_emulation` runs AmigaDOS scripts in Amiberry – without a
window, isolated (own working directory, own XDG directories, read-only
hardfile), hence also in parallel: `amiga_emulation_start()`
(configuration = the machine's template plus changes, script, time
limit; machines: `a1200` = `default.uae`, `a500` = `a500.uae` with
Kickstart 1.3, `a500-68010`),
`amiga_emulation_command()` (control socket of Amiberry, e.g.
`SCREENSHOT`, `READ_MEM`), `amiga_emulation_join()`. The script sees its
directory as `TEST:`. By hand:

```
echo 'Version >TEST:v.txt' | build/amiga/host/tools/amiga_run/amiga_run -
amiga_run [--machine NAME] [--rtg] [--visible] [--log] [--timeout SEC]
          [--set KEY=VALUE] [--unset KEY] [--mount VOL=DIR] [--mount-ro VOL=DIR] [--dir WORKDIR]
          SCRIPT|-
```

`--log` writes Amiberry's own log into `amiberry.log` of the working
directory (e.g. with `--set log_illegal_mem=true`: every access to
unmapped memory as `Illegal bget at ADDRESS PC=...`).

Requirement: Amiberry ≥ 8.4 in `PATH`, Kickstart ROM and hardfiles in
`amiga-emulation/` (see the README there).

The first configure writes `.clangd` (from `cmake/clangd.in`, with the
include paths of the toolchain found; not in the repository). After
changing the toolchain, delete `.clangd` and configure again; switch off
with `-DBLITZ2_CLANGD=OFF`.

## Structure

```
CMakeLists.txt, CMakePresets.json
archive/                     unchanged originals (sources, reference data),
                             see archive/README.md
cmake/m68k-amigaos.cmake     toolchain (the only one)
cmake/BlitzLibrary.cmake     blitz_library(), biglib_join()
programs/blitz2/             compiler (src/: sections of blitz2.s + glue)
programs/ted/                editor (src/: Amiga part, src/core/: OS-free
                             core)
libs/include/                blitzlib.h & co. (conventions of the libraries)
libs/variants/               the variants of the libraries (NAME/libnums.h:
                             the library numbers; bb2-2.10 the default)
libs/userlibs/               userlibs of a variant (rwe/: Red When Excited,
                             variant rwe-1999; rievallib: rwe-1999-eval,
                             risoundlib2: rwe-1999-ahi)
libs/blitzlib.ld             linker script: one code hunk, header at offset 0
libs/acidlibs/               one C file per library
libs/deflibs/                one C file per AmigaOS library
libs/tests/                  libs_tests: the libraries against their
                             originals in the emulator (m68k, snitch);
                             variants/: the test variants, check.cmake
shared/filereq/              file requester of freqlib and TED (one source,
                             compiled by both with their own options)
shared/edstub/               frame for programs that blitz2 loads as
                             blitz2:ted (entry points, comdata, no screen)
shared/xtra/                 the companion settings of a source (.xtra)
                             as one structure: Blitz 2 .xtra, BB22
                             header, AmiBlitz3 embedded header (C99, no
                             libc; compiled by each user, for later use
                             in blitz2 and TED; m68k compile-only now)
host/                        the host side: everything built with the
                             native compiler (tools, host tests), as
                             ExternalProject (build/<preset>/host)
programs/ted/tests/host/     host tests of the TED core (snitch)
programs/ted/tools/          host tools on the TED core: tedtok (tokenize
                             like TED), teddetok (source as TED shows it;
                             target bblibsrc)
programs/ted/tests/oracle/   TED oracle: original TED as a library (m68k)
tests/data/                  generated test data (oracle hashes, token list,
                             CD reference of blitz2 2.10)
tests/ted-ab/                TED A/B: original TED against TED-C in the
                             emulator; amiga/: teddrive, teddrivetest,
                             tedprobe
                             scripts/: the tests (.drv), src/: sources,
                             runner/: tedab (the runs), teddump
                             (screen dumps), both host; host/: tests
tests/corpus/                corpus: blitz2-c against blitz2 2.10 in the
                             emulator; amiga/: tedstub (blitz2:ted for
                             batch runs), reqtrap, reqclose, reqclosetest
                             tests*/: the test programs (plain text, .bas),
                             runner/: corpusrun (one pass), cdrun (the
                             CD through a compiler), both host
tools/                       host tools:
  lib/                       the logic as libraries: hunk (read the hunk
                             format), blitzlib (header, token table,
                             check rules), biglib (write/read BigLib),
                             blitztokens (the token list of compiler
                             and libraries),
                             hunkcheck (bytes in the first hunk),
                             amiga_emulation (scripts in Amiberry),
                             vasmlisting (read vasm listings), sha256,
                             checklabels (labels of ted.s in the C code),
                             checkdata (data against the reference build),
                             asmcmp (assembler routines against originals),
                             mktestdata (binary test data from a text
                             description: 8SVX, ILBM, modules),
                             bb22 (the BB22 source format of RWE, in
                             TED's format),
                             blitzres (residents: constants written,
                             contents read; NDK FD files and includes),
                             bblines (line edits of tokenised sources),
                             xtra (shared/xtra compiled for the host),
                             teststats (the @stat line of the emulator
                             sessions, statistics of JUnit files);
                             blitzlib also compares declarations
  biglib/, blitzlibcheck/,   the tools: one main.c each around lib/
  blitzlibcompare/,
  blitztokens/
  hunkcheck/, amiga_run/,
  vasmlisting/, checklabels/,
  checkdata/, asmcmp/,
  mktestdata/, bb22/, blitzres/,
  bblines/, xtra/, teststats/
  tests/                     tests of lib/ with inputs from memory
```

The libraries are compiled with the Blitz conventions
(`-ffixed-a3/a4/a5`: strings, locals and globals of the Blitz program;
freestanding; one hunk), blitz2 and TED as normal programs with
`-fbaserel` (a4 = base of their global data). That is why
`libs/acidlibs` (freqlib) and `programs/ted` each compile the shared file
requester themselves; each side provides its own `filereq_env.h` for it.

## License

GPL-3.0-or-later (`LICENSE`) with a runtime exception
(`RUNTIME-LIBRARY-EXCEPTION`): programs compiled by blitz2 may distribute
the code that blitz2 copies into them from the libraries (`acidlibs`,
`deflibs`) and its code templates under any license; the compiler, editor
and libraries themselves remain under the GPL.

The basis is the source code of Blitz Basic 2 and TED 1.1, which Acid
Software released as "under GNU" (without a version number – according to
GPL 2 § 9 one may then choose any version). The exception is granted by
the contributors of this project for the rights they hold; for the
translated original parts nobody can grant it bindingly, since the rights
holders cannot be reached. What is known about the license of the
originals is in `archive/README.md`.

