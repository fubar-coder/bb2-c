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
list `RWE_LIBS` there names per library the Blitz source and the CD binary
the C build must equal (ctest `rwe_identical_NAME`, `rwe_numbers`). Target
`rwelibsrc`: those Blitz sources as TED shows them, in
`build/amiga/rwelibsrc/` (like `bblibsrc`).

## Checks in the build

The build checks its results itself; if a check fails, the result counts
as not built, and nothing that depends on it is built (the next run checks
again):

* every library file (`tools/blitzlibcheck`, in the same step as link and
  strip): one code hunk, relocs only into it, library header, token table
  readable up to the end. References from the normal code into the error
  check area are only a warning (without the debugger they point behind
  the end of the code; the originals do that too). `acidlibs`/`deflibs`
  are made only from checked libraries.
* `acidlibs`, `deflibs` (also of every variant), in the same step as the
  join, and every userlib in its build step (against the acidlibs and
  deflibs of its variant): no library number twice (`biglib check`; deflibs together with
  the acidlibs loaded with it) – two libraries with one number would have
  the same tokens.
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
  here; `tokens_broken` checks that a missing library is noticed. The host side on its own:
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
  `build/<preset>/tests/corpus/NAME.txt`. Do not start the passes in
  parallel with `ctest -j`: each one itself uses up to
  `CORPUS_PARALLEL` emulators. `corpus_cd`: all 724 sources of the CD
  compiled with blitz2-c (like bb2's bb2corpus: `.xtra` of the sources,
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
  faults (no free signal, failing AllocMem of a size, Read error, failing
  Examine, failing OpenLibrary of a name, failing CreateProc, recorded
  allocations and FreeMem of another size than allocated; with `pad` 64
  zeroed bytes behind every allocation, also a hunk of LoadSeg, and a
  FreeMem inside an allocation is reported and not done; for the
  compiled programs with `runwrap=@INJECT@|...`) and reports what the
  command leaves behind (library opens minus closes of the process, its
  current directory and whether it unlocked the start lock, paths still in
  use); it also gives a program input while it runs: gadget clicks as
  synthetic IntuiMessages (`click=TITLE/TEXT`, optionally while the DOS
  list is locked for writing) and several RAWKEY events in one call of
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
  longer than 10240 bytes), `macrobuf=N` their macro buffer;
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
  configurations judged) and `ID.expect` (the correct behaviour; a line
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

## Emulator

`tools/lib/amiga_emulation` runs AmigaDOS scripts in Amiberry – without a
window, isolated (own working directory, own XDG directories, read-only
hardfile), hence also in parallel: `amiga_emulation_start()`
(configuration = `default.uae` plus changes, script, time limit),
`amiga_emulation_command()` (control socket of Amiberry, e.g.
`SCREENSHOT`, `READ_MEM`), `amiga_emulation_join()`. The script sees its
directory as `TEST:`. By hand:

```
echo 'Version >TEST:v.txt' | build/amiga/host/tools/amiga_run/amiga_run -
amiga_run [--rtg] [--visible] [--log] [--timeout SEC] [--set KEY=VALUE]
          [--unset KEY] [--mount VOL=DIR] [--mount-ro VOL=DIR] [--dir WORKDIR]
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
                             variant rwe-1999)
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
                             description: 8SVX, ILBM, modules);
                             blitzlib also compares declarations
  biglib/, blitzlibcheck/,   the tools: one main.c each around lib/
  blitzlibcompare/,
  blitztokens/
  hunkcheck/, amiga_run/,
  vasmlisting/, checklabels/,
  checkdata/, asmcmp/,
  mktestdata/
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

