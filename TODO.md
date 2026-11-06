# TODO

As of 2026-10-28. This repository is the reference for the code
(decision of 2026-10-27; takeover from bb2 `7b5df74`). Build, reference
builds, oracle and all tests are here now (§1, the open points are noted
there); only bb2's analysis tools in Python were not taken over. Goal: all
checks that bb2 had before Python was removed active again – here in the
build or as ctest. Only after that the switch to C23 (§3).

## 1 Checks still missing compared to bb2

Already active again (here): library files (`tools/blitzlibcheck`,
formerly `mkblitzlib.py`), TED's hunk layout (`tools/hunkcheck`, formerly
`checkhunk.py`), `checklink`; tests of the host tools.

**TED (bb2: `ted-c/`)**

1. [x] **Reference build of `ted.s`** done (2026-10-27): target
   `ted_ref`, original sources in `archive/sources/ted-1.1/`,
   `tools/vasmlisting` instead of `tedlisting.py` (`map`, `labels`,
   `verify`); `ref/ted`, listing, map and labels byte-identical to bb2;
   `verify` is a check in the build step.
2. [x] **`checkdata`** done (2026-10-28): `tools/checkdata` (instead of
   bb2 `tools/checkdata.py`, output identical line by line), table
   `programs/ted/checkdata.tsv` (534 entries), POST_BUILD of `ted.sym`
   (second link with symbols as in bb2); `ted` is linked only after that.
   A missing C symbol is now an error (in bb2 "todo").
3. [x] **`checklabels`** done (2026-10-28): `tools/checklabels`
   (instead of bb2 `tools/checklabels.py`, output byte-identical) as
   POST_BUILD of `ted`: 278 code labels, all exactly once.
4. [x] **Host tests of the TED core** – done (2026-10-28): all 10 tests of
   bb2 `ted-c/tests/host` ported to snitch (`programs/ted/tests/host`,
   core as `tedcore_host`, helpers `lines_helpers.hpp`, `tempdir.hpp`,
   `oracle.hpp`, `corpus.hpp`). The corpus tests detok, tokit, loadsave
   compare exactly by SHA-256 with the original TED
   (`programs/ted/tests/oracle`, frame `shared/edstub`, target
   `ted-oracle` → `tests/data/ted-oracle.sha256`, 724 sources, 715
   tokenized; token list `tests/data/ted-tokens.txt`), without the
   bb2tok exceptions; skipped without the CD. `TED_TEST_DUMP=DIR` writes
   deviating results for comparison with an oracle run. Checked against
   bb2tok: load/save of all 724 equal; detok/tokall deviate in 84 files
   only for unknown tokens (`<$XXXX>` versus `?????`). The token list of
   the compiler has the same 1821 entries as bb2tok, but in a
   **different order** (bb2tok: built-in, deflibs, acidlibs).
5. [x] **TED A/B tests in the emulator** (bb2: `tools/tedab.py`, `teddump.py`,
   `tests/emu/*.drv`, Amiga tools `teddrive`, `teddrivetest`, `tedprobe`
   in `tests/amiga`): original TED against TED-C, with blitz2 1.95 and
   blitz2-c. In addition the host tests of the tools (`test_tedab.py`,
   `tdscript_host`).
   Decided (2026-10-28): `teddrive` remains the driver in the guest.
   Amiberry's remote control (`SEND_KEY`, `SEND_MOUSE_ABS`, `READ_MEM`,
   `MONITOR tasks`, `SCREENSHOT`) could replace it, but only as a rewrite:
   the host would have to read windows, requesters, gadgets and menus from
   the Intuition structures, hit menus and gadgets with the mouse, obtain
   dumps from PNG or memory. The advantages (no test program in the guest,
   real input path) are too small, the outcome is uncertain.

   **Plan (2026-10-28, agreed):** location `tests/ted-ab/` (cross-cutting:
   TED plus blitz2, shares `reqtrap`, sources and runner parts with the
   corpus); one ctest `ted_ab` (label emulator, internally parallel up to
   `CORPUS_PARALLEL`, selection via a filter).
   1. [x] Amiga tools done (2026-10-28): `tests/ted-ab/amiga/`
      `teddrive`, `teddrivetest`, `tedprobe` (byte-identical to bb2;
      `teddrive` includes `tests/corpus/amiga/reqclose.h`); host test of
      the script parser `tdscript.h` as snitch (`ted_ab_tdscript`).
   2. [x] Dumps on the host done (2026-10-28): `tests/ted-ab/runner/`
      `teddump` (library `lib_teddump` and tool `teddump info|text|cmp`):
      decoding with the fonts from `archive/sources/ted-1.1/`, masking
      "Free", comparing; SHA-256 instead of SHA-1, the deviating lines
      instead of a unified diff; no PNG for now. Test `ted_ab_teddump`
      after bb2's `test_teddump.py`. On bb2's 150 real dumps: text
      byte-identical to `teddump.py`, all 124 pair comparisons judged the
      same.
   3. [x] Runner `tedab` in C done (2026-10-28): `tests/ted-ab/runner/`
      (`lib_tedab` + `tedab`, pass file from CMake), scripts to
      `tests/ted-ab/scripts/`, sources `E`/`S` to `tests/ted-ab/src/`.
      Expectations without bb2 paths: `corpus` (program of the corpus ref
      run), `tedsave` (oracle hash `save/`), `cd` (file of the CD); `tok`
      via the `*.expect` files tokenized at build time with `tedtok`. `E`
      is provided as text (TED tokenizes itself), `K` is searched on the
      CD case-insensitively. Sessions in parallel up to
      `CORPUS_PARALLEL`, runs not reached after a hang once more.
      Requester sequence and lock evaluation checked against bb2's data
      (79 logs, lock report equal). New compared to bb2: all four
      combinations in one session (budget 60 s, limit 120 s instead of
      25/60 s). blitz2's compile window (`STOP!`) is seen by `teddrive`
      0–2 times depending on timing; the postfix `; @ignore
      compilewindow` on a script line removes it from the requester
      sequence in this step (a comment: teddrive skips it, its log
      contains it; `ted_ab_lint` checks it). Set on the `freq ok` steps
      of `06-compile`, `07-error`, `29-cmp-*`.
   4. [x] ctest `ted_ab` (label emulator, fixture on `corpus`; filters
      `TED_AB_FILTER`, `TED_AB_COMBOS`) and `ted_ab_lint` (without
      emulator). As of 2026-10-28: 43 tests, 168 of 172 runs OK, all 168
      comparisons between OK runs equal – stage 1/2a as in bb2, plus
      `09-opts` (now also Cc/Rc), 24–29 (not run in bb2). Open:
      `23-font6` (KNOWN-BOTH, confirmed in the emulator: exception
      catching and instruction trace via Amiberry's control socket):
      `calcscreen` makes the font 6 screen 513 lines high on a 512-line
      Workbench, OS 3.2 opens it with 512, `OpenWindow` fails, the error
      path of `screenopen` crashes (BUG(orig) survey-05 I.1, reproduced in
      TED-C; newly noted as BUG(orig) in `calcscreen`). The test can never
      succeed this way; it therefore carries `@known` (not run, listed in
      the report as KNOWN with a reason; can still be run by its exact
      name in the filter).

**Libraries (bb2: `libs-c/`)**

6. [x] **Instruction comparison of the assembler routines** done
   (2026-10-28): `tools/asmcmp` (instead of bb2 `tools/asmcmp.py`, output
   equal, also for deviations), ctest `asmcmp_runerrlib`,
   `asmcmp_debuglib`, `asmcmp_memacclib` with the routine lists from bb2;
   originals from the CD (`BLITZ2/ACIDLIBS`, equal to bb2 `ref/*.obj`),
   symbols from `NAME.linked` (`HUNK_SYMBOL`, no `nm`).
7. [x] **Declaration comparison with the originals** done
   (2026-10-28): `tools/blitzlibcompare` (rules of bb2 `biglib.py
   compare`, previously by hand), ctest `declarations_acidlibs`,
   `declarations_deflibs` against `BLITZ2/ACIDLIBS`, `DEFLIBS` of the CD:
   83 + 39 libraries, declarations identical, same order (cross-checked
   with `biglib.py compare`: 122 identical).
8. [x] **snitch tests in the emulator** done (2026-10-28): `libs/tests/`
   (`libs_tests`, m68k snitch program from bb2 `libs-c/tests`, unchanged
   except for comments), ctest `libs_emulator` (`emulator.cmake` instead
   of `tools/amiga_test.py`): every acidlib of this build against its
   original, which `biglib extract` fetches at test time from
   `BLITZ2/ACIDLIBS` of the CD (equal to bb2 `ref/*.obj`). 7 test cases,
   19372 assertions as in bb2, about 18 s; output equal to bb2's
   reference log except for one stack address. A changed byte in
   `ldivlib` makes the test fail.

**Compiler and the whole (bb2: `compilerlauf/`)**

* Reference build of `blitz2.s` 1.95 done (2026-10-28): target
  `blitz2_ref`, sources in `archive/sources/blitz2-1.95/`; `ref/blitz2`
  byte-identical to bb2 `compilerlauf/blitz2-195`, `verify` 13592 lines.

9. [x] **Corpus** done (2026-10-28, phases 1-5 below): bb2's
   `tools/corpus_check.py`, `bb2corpus.py`, `bb2compile.py`,
   `serialrun.py`, `corpus/`, `tedstub`, `reqtrap`, `reqclose` are here in
   C/CMake (`tests/corpus/`); `ctest --preset emulator` about 3 minutes.

   **Plan (2026-10-28, agreed):** location `tests/corpus/`
   (cross-cutting).
   1. [x] Amiga tools done (2026-10-28): `tests/corpus/amiga/`
      `tedstub` on `shared/edstub` (batch mode only; a single job is a
      list with one entry), `reqtrap`, `reqclose`, `reqclosetest`
      (byte-identical to bb2). Trial run with blitz2 1.95: results, error
      line and hashes as in bb2 `reference/195.tsv`.
   2. [x] Test sources done (2026-10-28): `tests/corpus/tests*/*.bas`
      (135); tokenized with `tedtok` (C core of TED, `fileopen2` +
      `realsave`, real token list), which rejects text lines over 129
      characters (TED truncates them). 15 such lines in 9 sources from
      bb2 split (`NPrint a,…,z` → `Print a,…` / `NPrint …,z`, same
      output); after that all 135 byte-identical to bb2tok. Instead of
      bb2's `default.xtra` (origin unclear) the stub with `runerr=off`
      writes the startup options of the compiler without error
      checking/debugger.
   3. [x] Check matrix done (2026-10-28): `tests/corpus/runner`
      (`corpusrun`, C on `amiga_emulation`), ctest `corpus`,
      `corpus_runerr`, `corpus_req` (label emulator): **201 checks, 0
      deviations**, about 100 s (3 emulators in parallel; serially about
      three times as long). clib = C acidlibs and C deflibs. The programs
      load sounds and modules from the CD via `BB2:` (as in bb2). Program
      outputs byte-identical to bb2's runs except for `t13_env` (reads
      the environment on purpose: Workbench 640×512×8 instead of
      640×256×2).
   4. [x] CD reference run done (2026-10-28): `tests/corpus/runner/cdrun`,
      target `blitz2-cd-reference` (blitz2 1.95, about 35 s) →
      `tests/data/blitz2-cd.tsv`: 498 compiled, 217 errors, 9 text
      sources; all 724 lines equal to bb2 `reference/195.tsv`,
      reproducible. ctest `corpus_cd` (blitz2-c): 724 equal, about 30 s.
   5. [x] Serial test done (2026-10-28): `tests/corpus/runner/serial.c`
      (peer with POSIX sockets and pthread, protocol as in bb2's
      `serialrun.py`), pass key `serial=on`: per configuration a separate
      emulator with its own port (`serial_port=TCP://127.0.0.1:PORT`) and
      its own peer; the bytes sent over the serial port are compared as
      well, a `TIMEOUT` in the output counts as a deviation. ctest
      `corpus_serial`, `corpus_serial_runerr` (the latter also checks:
      the reference sends the same as without error checking):
      `t64_serialrx` 0 deviations, about
      17 s each. bb2 kept only the report of its serial runs, a byte
      comparison with it was not possible.
   Parallel emulator runs only with a **configurable upper limit** (1
   must always work): Windows Amiberry uses numbered named pipes, macOS a
   fixed socket under `/tmp`; today's isolation via `XDG_RUNTIME_DIR`
   exists only under Linux.

**Basis for 5, 8, 9: emulator library `tools/lib/amiga_emulation`**
– **implemented** (2026-10-27, with `tools/amiga_run` and tests: four
runs in parallel, RTG, control socket, time limit with screenshot). First
user: the TED oracle (`ted-oracle`, `amiga_run` with additional drives).
Corpus (`corpusrun`, `cdrun`), TED A/B (`tedab`) and the snitch tests of
the libraries (`libs_emulator`) now run here via `amiga_emulation`
(2026-10-28), no longer via bb2's `tools/amiberry.cmake`.

* Default configuration `tools/lib/amiga_emulation/default.uae` (from the
  AGA configuration, cleaned up; RTG variant = changes) and
  `amiga-emulation/` with `AMIGA-EMULATION-SETUP.md` (expected files, checksum of the ROM,
  conditions) are in place (2026-10-27); the user has stored the ROM and
  the adapted hardfiles, both images checked. Open: CMake reports
  missing files.
* The drive of the tests is called **`TEST:`** (no longer `AB3C:`, that is
  a different project); both hardfiles are adapted (2026-10-27). Proposal
  for the `user-startup` (query made safe – a drive that is not mounted
  would otherwise give the requester „Datenträger einlegen“ (insert
  volume)):

  ```
  Assign >NIL: TEST: EXISTS
  If NOT WARN
    If EXISTS TEST:run.script
      Execute TEST:run.script
    EndIf
  EndIf
  ```
  Remove today's lines for `SPEC:`, `AB3C:` and `Assign add Blitz3: AB3:`
  or make them safe in the same way.
* Mount the **hardfile read-only** (`hardfile2=ro,…`,
  `uaehf0=hdf,ro,…`): checked on 2026-10-27 – boots, `T:`/`ENV:` are in
  RAM, two instances share the same image, the image stays unchanged. A
  write attempt to `SYS:`, however, opens „Datenträger Workbench ist
  schreibgeschützt“ (volume Workbench is write-protected) and hangs: tests
  write only to `TEST:` or `RAM:` (catch requesters: reqtrap from bb2 or
  time limit).
* **Parallel:** per run its own `TEST:` directory and its own `XDG_*`
  directories including `XDG_RUNTIME_DIR`; two instances ran at the same
  time. Then ctest can run emulator tests with `-j`.
* **Control socket** (Amiberry 8.4, `$XDG_RUNTIME_DIR/amiberry.sock`):
  one text line per command, **arguments separated by tabs** (with spaces
  the whole line is read as the command name and converted to upper
  case), answer `OK`/`ERROR` + tab fields. Checked: `GET_STATUS`,
  `READ_MEM 4 4` (ExecBase), `SCREENSHOT <path>` (PNG, also without a
  window), `SET_WARP 1`/`GET_WARP`, `DEBUG_STATUS`, `QUIT` (quits
  immediately – replacement for `uaequit`). According to the help also
  `SEND_KEY`, `SEND_MOUSE`, `WRITE_MEM`, breakpoints,
  `SAVESTATE`/`LOADSTATE`.

**Principles for this**

* Tests live here with their component (e.g. `programs/ted/tests/`,
  later `libs/acidlibs/tests/`), cross-cutting ones up in `tests/`;
  generated test data in `tests/data/` (implemented this way,
  2026-10-28).
* Reference data: original sources in `archive/sources/`, CD and Aminet
  archive via `fetch-originals` (original libraries:
  `archive/cd/…/BLITZ2/DEFLIBS`, `ACIDLIBS`).
* The complex Python tools stay Python for now (decision of
  2026-10-27); here only C/C++ and CMake. A test that needs Python
  therefore stays in bb2 for now.

## 2 Further open items

- [ ] **`defaultdbug` built by the port** (user decision 2026-11-06):
  blitz2 loads `blitz2:defaultdbug` for programs run from the editor with
  runtime error checking; without it such a program ends silently at its
  first runtime error. The Aminet sources' `bblibsrc/dhandler.bb2`
  (fetched by `cmake/fetch-originals.cmake`) compiles with `blitz2-ab3`
  in BB2 mode, and the result works as `blitz2:defaultdbug` and with LOAD
  DEBUG MODULE (checked in Amiberry, Workbench 3.1). For now the website
  describes how to build it by hand. Next: a target `defaultdbug` outside
  the normal build that compiles it with `amiga_run` (needs the
  originals and the emulator), so release builds can contain it.
  Translating the debugger to C is a project of its own, after the port.
- [x] **`__saveds` instead of hand-written a4 loads** (user decision
  2026-10-29): since m68k-amigaos-gcc 16.2-rc21 (AmigaPorts/gcc#141)
  `__saveds` loads a4 again (`jsr ___restore_a4`); older versions accept
  it and generate nothing - wrong code without a warning. Steps:
  1. [x] toolchain check done (e6e3dbb): configure compiles
     `cmake/saveds/good.c` and needs `jsr ___restore_a4`; test
     `toolchain_saveds_broken`; README and CLAUDE.md: gcc ≥ 16.2-rc21
  2. [x] `runtheprog` done (3a572c3): `__saveds` instead of the
     `runtheprog_a4` stub; blitz2 changed (intended), checked with
     `ctest --preset all` and the bug-orig tests with Compile & Run
  3. the other `GLUE_LOAD_A4` places (blitz2 `glue.c`, `glue_direct.c`;
     TED `glue.c`, `glue_start.c`): recommendation, not done - they are asm
     thunks that save all registers with movem and pass the frame to C;
     `__saveds` on the C function would only replace the one `lea`, save
     a4 a second time and call `___restore_a4`; the thunk stays anyway.
     `directdo` is entered from an exception (rte): the explicit `lea`
     is the clearer form there. Decided (2026-10-30): left as they are –
     the code is assembler anyway and moving it to C is not worth it.
- [x] **`-h` / `--help` for all callable programs** done (2026-10-29,
  request of 2026-10-28): usage text with a line on what the program
  does, to stdout, return 0. Host (61736be): the tools, tedtok, teddetok,
  corpusrun, cdrun, tedab, teddump - tests `help_*`
  (`cmake/check-help.cmake`). Amiga (2d19438): reqtrap, reqclose,
  reqclosetest, inject, wbrun, teddrive, teddrivetest, tedprobe - test
  `help_amiga` in Amiberry. Not included: `tedstub` and `tedoracle`
  (only blitz2 loads them as `blitz2:ted`), and `blitz2` and `ted`
  themselves (decision of 2026-10-28: they reproduce the originals,
  argument = file; a help would be a `DIFF`).

- [x] **Git history** decided (2026-10-28): this repository is the
  reference and stays with the fresh start; the history of the files is
  not taken over from bb2 (it remains there). Remote: `origin`
  (GitHub `fubar-coder/bb2-c`).
- [x] **Port `dhandlerlib` (lib 1)** done (2026-10-29):
  `libs/acidlibs/dhandlerlib.c`, byte-identical to the CD's library
  (header in C, the rest assembler: process segment template, relocation
  order; `asmcmp_dhandlerlib`); `prebuilt/` removed.
- [ ] **Readable names** (user decision 2026-10-30, before the error check
  area fixes below). Stage 1 is done: a pure rename, every output
  byte-identical. It covered blitz2, TED and the libraries (2026-10-31:
  `libs/acidlibs`, `libs/userlibs/rwe`; `libs/deflibs` needed none).
  Stage 2 splits the variables that hold several unrelated values (one
  variable per value) and renames the file-scope statics kept in stage
  1. It is not byte-identical, so it is checked with the tests and
  named in the commit. The list is in `docs/libs/variable-split.md`;
  done for both parts (2026-10-31): acidlibs and userlibs/rwe, with the
  variables left as they are where the original depends on the reused
  register (listed there).
  The register-level translations (2dlib, medlib, polylib: static
  `d0`-`d7`/`a0`-`a6`) are a rewrite of their own after stage 2; done
  2026-10-31 (plain C, checked with the corpus incl. t158/t159 and
  libs/tests/systest_med.c).
- [ ] **Data blocks as structs** (user decision 2026-10-31, after stage 2
  of the readable names): data blocks of the originals that the port
  addresses as arrays or with offsets (tag lists, NewScreen/NewWindow
  blocks, requester data, the libraries' own variable blocks) become
  structs with named members (the system's structs where they exist,
  `static_assert` on the size). Byte-identical: the original's wrong
  offsets stay, now visible as the wrong member (example: risoundlib2's
  `ahireqtags`, #931/#934); a bug that shows up this way gets an issue
  and a `BUG(orig)` note, and is fixed later like the others.
- [ ] **A type for Blitz booleans** (user decision 2026-10-31, with the
  readable names): `typedef enum : ULONG { BB_FALSE = 0, BB_TRUE =
  (ULONG)-1 } BB_BOOL;` (C23, fixed underlying type) in a shared header
  (`libs/include/blitzlib.h`). It is exactly a ULONG: d0, `BL_REG` and
  the outputs stay the same (byte-identical). Functions that give or take
  a Blitz truth value use `BB_BOOL`, `BB_TRUE`/`BB_FALSE`. Error values,
  counts and pointers stay as they are: of the ~190 places with
  `(ULONG)-1`/`-1` in the libraries, each is checked on its own. exec's
  `TRUE` is 1, not Blitz's -1: it stays for the internal BOOLs only.
  Checked 2026-10-31: the 7 uses of `TRUE` are comments (arexxlib,
  crunchlib) or internal (memlib `freelast_c` tested as Z, rifxlib
  `zoomstart`, rianimlib `unpackanim`'s parameter); none reaches a
  Blitz program. Checks: gcc's `-Wc++-compat` warns about int → BB_BOOL
  (`return x`, `return 1`), clang's `-Wassign-enum` /
  `-Wimplicit-int-enum-cast` too (clangd); `b == 1` and `return BB_TRUE`
  in a function returning ULONG are not caught in C (enum → integer is
  always allowed). In C++ an `enum class BB_BOOL : ULONG` would refuse
  both directions and still be a plain integer in d0 (no struct
  needed) - an option only if the libraries move to C++.
- [ ] **Function documentation** (user decision 2026-10-31, after stage 2
  of the readable names, before the error check area fixes): Doxygen
  comments (`/** @brief … @param … @return … */`), which clangd shows on
  hover. They cover:
  - the public library functions, i.e. the Blitz commands: what the
    command does, the parameters with their registers (d0, d1, …), the
    result, error cases and `BUG(orig)` notes with their issues. The
    Blitz syntax stays in `BLITZ_NAME`;
  - the quasi-libraries with the same form: `libs/userlibs/rwe/optidec.h`,
    the code in `shared/` (xtra, filereq, …) and `tools/lib/`.
  Comments only, every output byte-identical. Possible later: a check
  with clang's `-Wdocumentation` in the host build (an `@param` that does
  not match the parameters), and an optional `doxygen` target outside
  the build.
- [x] **Warnings of `blitzlibcheck`** done (2026-10-31): all 73 sites per
  acidlibs set fixed (displaylib #412; screenslib #251, animlib #398,
  iffmakelib #185, fontlib #955, memlib #956, trackerlib #957,
  wbstartuplib #305; ilbmifflib #192, audiolib #70, shapeslib #270 and
  #953, spriteslib #954), each with a bug-orig test that comes out FIXED
  (wbstartuplib: not testable, the error is in the library's init);
  rigtmenulib's three were a port defect (check functions in front of
  the thunks). A clean rebuild has no blitzlibcheck warning left, and
  such a reference is now an error of the check; the fixes are kind F in `docs/libs/deviations.md`. Was: 73 warnings per
  acidlibs set (and three of rigtmenulib): normal library code refers into the error check
  area – a command that fails at run time (e.g. `Screen`: `OpenScreen`
  fails, `BEQ err1`) jumps to an error routine there, or raises an error
  with a text there (screenslib `err12`). Without "Runtime Errors" blitz2
  leaves the area out, so the jump goes behind the library. Original bugs
  with their issues (e.g. #70, #185, #192, #251). The fix (once original
  bugs are fixed, CLAUDE.md): the error routine and its text move into
  the normal code of the command; the checks the compiler calls before a
  command (`!subs`, e.g. `prescreen`) stay in the area. Without the
  debugger the moved routine raises `TRAP #0` as before (decision
  2026-10-30: a clean crash, no silent error value). The fixes go into the
  default variant (decision 2026-10-30: byte equality with the CD's
  originals mattered only for the port). Each fixed site removes its
  warning.
- [x] **License header lines** decided (2026-10-28): no SPDX lines in the
  source files; the license applies via `LICENSE` and the README.
- [x] **CD data** solved (2026-10-28): not in the repository,
  `cmake/fetch-originals.cmake` loads the CD (archive.org) and the Aminet
  archive with a fixed SHA-256 to `archive/cd/`, `archive/aminet/`.
- [x] **Documentation `docs/`** done (2026-10-28): paths, commands and
  line references (C code: determined anew by content) adapted to this
  repository; history of bb2 (plans, logs, reviews) unchanged in content,
  remaining bb2 mentions explained in `docs/README.md`.
- [ ] **Source compatibility with AmiBlitz3** (user decisions
  2026-10-31, plan `docs/ab3/variant.md`): `blitz2-ab3` compiles sources
  recognised as AmiBlitz3's with AmiBlitz3 v3.14.0's binary libraries;
  `blitz2`'s results stay byte-identical (rule of 2026-11-01); since
  2026-11-01 both programs share one core (`blitz2_core`, extension
  points in `ext.h`, change `blitz2-ab3-extension-points`).
  1. [x] G1 done (2026-11-01, OpenSpec change `ab3-mode-g1`): build,
     command line with `ab3cli`, detection, AB3 token table, libraries,
     `^`, residents, text includes; tests `ab3_*`, `corpus_cd_ab3`.
  2. [x] G2 done (2026-11-01, OpenSpec change `ab3-mode-g2`): AB3
     tokeniser `shared/tokenizer`, AmiBlitz3's include rules,
     `OPTIMIZE`, `SYNTAX`, `RunErrs*`, `BSS`/`Chip`, new syntax, `@@`,
     global `SHARED`, `_` names, `.name`, `+`, `:@`, `#__include`/
     `#__debug`/`CNIF @`; 47 of AmiBlitz3's 68 test programs compile
     (AmiBlitz3: 57; `docs/ab3/variant.md`, "Measured share"); still 47
     after `ab3-pointer-assign` (`exec_data` now refused as in AmiBlitz3)
     and `ab3-function-statement` (`jsr` compiles): the same result as
     AmiBlitz3 for 58.
  3. [ ] G3–G5 (escapes, optional parameters, `Null`, casts, `Dim KEEP`,
     `Data` constants), then G7 (68020), then G6 (FPU). G3 done
     (2026-11-04, change `ab3-mode-g3`): escapes in strings and `Dc.b`,
     `@"…"`; 48 of the 68 test programs compile, as after G2. G4 done
     (2026-11-05, change `ab3-mode-g4`): optional parameters, up to ten
     parameters (7-10 on the stack), `FAST`, NewType return types; 50 of
     the 68 test programs compile.
     Also open: the differences found against AmiBlitz3's compiler
     (`docs/ab3/variant.md`, "Differences"). The rule for `Stop` without
     the debugger (`int_tests`) is decided (2026-11-01, change
     `debug-constant`): BB2's behaviour in both modes, and the predefined `#__debug` for `CNIF #__debug … CEND`
     (`docs/blitz2/extensions.md`).
  4. [x] `68k_crash_newtype` (AmiBlitz3's tokenised test program) done
     (2026-11-03): blitz2-ab3 compiles it in BB2 mode (rc 0). The hang
     was the alert 8100 0005 at blitz2-ab3's end: its companion `.xtra`
     sets a Macro Buffer of 40000, and blitz2 freed `macrobuff` with that
     size instead of the allocated 8192 (original bug #336, fixed; test
     `ab3_cli` case `macrobuf_xtra`). AmiBlitz3 stops with "Garbage at
     the end of line" in line 1 because in CLI mode it takes a file
     without the extension `.bb`/`.bb2` for text (`CLIModeSupport.ab3`
     449-466 at `@58a11658`); 48 of the 68 test programs compile
     (`docs/ab3/variant.md`, "Measured share").
  5. [x] Checked 2026-11-01 whether TED's baserel data hunk (0x7c8 bytes
     of `.bss` without startup code) is cleared by LoadSeg: not on
     Kickstart 1.3 (only `HUNK_BSS` is), yes on 3.2 – port defect #963.
     Fixed 2026-11-01: `cmake/baserel-nostart.ld` stores the `.bss` of
     `ted` and `ab3cli` as zeros in the data hunk, `hunkcheck --stored`
     checks it after every link.
  6. [x] AmiBlitz3's pointer assignment rule done (2026-11-01, OpenSpec
     change `ab3-pointer-assign`): in AB3 mode a pointer takes no basic
     value other than long ("Can't convert types", independent of
     `SYNTAX`); `exec_data` now fails as with AmiBlitz3 (46 of the 68
     test programs compile). NewType left sides and the `SYNTAX 4` part
     stay for G8.
  7. [x] An AmiBlitz3 `.xtra` of a tokenised main source done
     (2026-11-04): a current AmiBlitz3 `.xtra` has 15 switch words
     (`docs/formats.md` §7); `ab3cli` handed it to blitz2's `loadxtra`
     without `-r`, which read the six buffer sizes 2 bytes late (16384 →
     `$40000000`), and with `-r` `xtra_load` refused it (set-up error).
     No loader is written (user decision 2026-11-04: AmiBlitz3's
     developer plans to drop loading tokenised sources): `ab3cli` does
     not use such a `.xtra` and prints `NAME.xtra: AmiBlitz3's format,
     its options are not used`; the compiler's own options apply, the
     mode (AB3) still comes from the `.xtra`. Tests: `ab3_cli` cases
     `ab3xtra_tok` and `ab3xtra_tok_r` (a 15-word `.xtra` with
     AmiBlitz3's default buffers, Macro Buffer 40000). An older 14-word
     `.xtra` (`68k_crash_newtype.xtra`) is read right and handed over as
     before (`macrobuf_xtra`).
  8. [ ] An AmiBlitz3 release without tokenised sources breaks the
     `ab3_cli_ref` cases that read a tokenised file: `inctok_ab3` (the
     include `inctok_ab3_inc.bb2` with AmiBlitz3's `.xtra`) and
     `inctext_bbname` (an include named `.bb`, read as tokenised by its
     name). At the next version bump of the reference AmiBlitz3: take
     them out of `ab3_cli_ref` or record the difference, and decide
     whether AB3 mode follows.
  9. [x] Include case for `@"…"` done (2026-11-04, tasks 4.2 and 6.3 of
     `ab3-mode-g3`): `ab3_cli`/`ab3_cli_ref` case `inc_longlong`
     includes AmiBlitz3's `longlong.include.ab3` (v3.14.0, fetched),
     prints 1234567 with `!ll_ToStr` and reads back an IFF header written
     with `@"…"` chunk IDs (`1234567`, `FORM 4`, `ILBM ILBM` in both). It
     was blocked by "Illegaly nested Condition" (a one-line `If … Then`
     in a macro body open to the end of the expanded line, #967); AB3
     mode now ends such a macro line with `: End If` as AmiBlitz3 does
     (task 6.2). `file.include.ab3` needs G4's optional parameters
     (`docs/ab3/variant.md`, "Strings"; G4 done 2026-11-05).
  10. [ ] Empty string as a library constant: an omitted string
     parameter passes the pointer -1 as AmiBlitz3 does (G4), which
     AmiBlitz3's `lastring` recognises (strings2Lib.ab3:115-117) and
     `libs/acidlibs/strings2lib.c` does not (it copies 0 bytes from -1,
     the same visible result). Afterwards: a real empty string from a
     library (as `Null` comes from `environlib`) instead of the -1; an
     intended deviation from AmiBlitz3's code.
  11. [ ] `End d0` (G5): AmiBlitz3 ends the program with the value of
     d0 as its return code (AmiBlitz3.ab3:22688-22691 sets `returncode`;
     the program end then saves d0 with `move.l d0,(a7)` and returns
     with `rts` instead of `moveq #0,d0 : rts`, 3626-3647 at
     `@58a11658`). The flag holds for the whole program, so every `End`
     returns d0 once one `End d0` is compiled - check whether to follow
     that. Not taken over: `End noa7` (22682, 22795: patches the start of
     the program end routine to `addq.l #4,a7`, so a7 is not reset to its
     start value; undocumented, unused in AmiBlitz3's sources, purpose
     unknown; not available in direct mode).
- [ ] **Tests for the issues labelled `needs-test`** (99 open issues at the
  start, 3 open on 2026-10-29: #14, #26, #42). A failing test is added with label
  `bug-orig` (`tests/README.md`, "bug-orig tests"), named in the section "Test" of the issue,
  and `needs-test` is removed (allowed for these issues, 2026-10-28).
  State 2026-10-31 (open with `needs-test`: #361, #366, #369, #655):
  - [x] #42 (a500-68010); a batch with the new tools (`inject
    allocfail`, the machines `a500`/`a600`, CD image with
    `uaescsi.device`): tests for #939–#942, #825, #647, #636, #524,
    #927, #666, #840, #579 (#824 had one).
  - [x] Second batch: #444 (`ciatrklib-load-readerror-unlock`, inject's
    `readfail` follows `OpenFromLock`, new `ownedunlock`), #356
    (`ted-donew-allocnull-ignored`: the 6th 10-byte allocation is NEW's
    `allocnull`); #14 and #26 had tests (3 of 3 runs REPRODUCED, the
    section "Test" of the issues updated).
  - [x] Label `untestable` (reason in the issue): #896, #675, #607, #469,
    #581, #384, #352, #917 (AllocBitMap of OS 3.2 fails for height 0
    and does not return for depth 0: the copy is never reached).
  - [x] Closed as `not-a-bug`: #848, #821, #806, #819, #440, #433 (a
    failing globalloc never returns: memlib's "Out of Memory!").
  - [x] #946 (bug-port: ricompactdisclib's SCSI CDReadTOC hung after a
    session without scsiflag): fixed, port regression test
    `tests/bug-orig/regression/ricompactdisclib-scsi-after-plain`
    (the new regression mode of the runner); the original's 26-byte
    CD_INFO is #948 (`untestable`).
  - [x] Flaky tests made deterministic: #28, #429, #873,
    #823 (`rishapeslib-savecmap-zero-colours`: the overwrite lands in a
    region of the program's own), #492 (`riamosfunclib-count-zero`: real
    A1200 speed without the JIT, the blocks at the top of memory, under
    Disable - at JIT speed the copy reached the hardware and Amiberry
    sometimes ended).
  - [ ] #361: the only-line path of `freeupa5` is not reached with the
    keyboard (a block covers a line to its end only with a line behind
    it); needs a mouse block to column 127 in teddrive.
  - [ ] #369: a blitz2 allocation through `extalloc` that fails only
    during Compile (size and count of the start-up allocations to be
    found), with a temporary fix on both sides.
  - [ ] #366: not reproducible on the reference (re-checked 2026-10-31
    with `alertlog`: no alert, no crash).
  - [ ] #655: deferred (needs two machines).
  - Done (2026-10-28), every test REPRODUCED and, except where noted in
    the issue, checked with a temporary fix (FIXED, fix reverted):
    - compiler, Blitz programs in `tests/bug-orig/compiler/`: #2, #3, #8,
      #16 (verdict DIFFERS: the port has other data behind `incdir`, BSS
      followed by `comdata`, so the port's compiler crashes already, not
      only the compiled program; noted in the issue), #22, #23, #27,
      #28, #336 (inject track reports the FreeMem of another size), #19,
      #20 (started like from the Workbench, `wbrun`);
    - libraries, Blitz programs in `tests/bug-orig/libs/` (judged with
      the C libraries, `; @configs clib`): #58, #67, #69, #80, #104,
      #57 (string buffer raised with tedstub `strbuf=`), #107 (started
      with an argument, `runargs=`),
      #110, #135, #137, #171, #177, #179, #189, #191, #203–#209, #211,
      #212, #224, #225, #245, #246, #263, #266, #268, #269, #274, #280,
      #281, #286, #289–#291, #293–#295, #299, #303, #304, #308, #310,
      #314, #316, #318, #319, #321, #322, #334, #306, #307 (`wbrun`); test data (8SVX, ILBM, MED,
      tracker modules) generated by `tools/mktestdata` from
      `libs/ID.*.spec`;
    - libraries, snitch tests in `libs/tests/bugorig.c`: #112, #115–#119,
      #127.
  - Closed as not planned (no test possible, reason in the issue): #5,
    #18, #32, #264 (its impact was corrected: without BODY the decoder
    fails with "Can't Read File" or crashes without error checking, like
    any load error, no shape with arbitrary contents), #65 (ASL returns
    font file names of at most 30 characters, a name over 95 is not
    reachable; input in the requester cannot be tested), #133 (no
    error raiser passes a number in d1, so Ern has no correct value; it
    looks like a planned feature that was never carried out), #4
    (`macrobuff` always has 8192 bytes, more than 1365 parameters do not
    fit in a line, nested macros add none).
  - *Formerly open, test programs* (both have tests now:
    `compiler-pushput-leak` with tedstub `objbuf=`,
    `compiler-flushlibs-leak` with 512 MB Z3 RAM):
    #14 (the "Not Enough Object Memory" path was not reached in the
    batch run, even with a 128 KB executable;
    a leak of 14 bytes once seen with `inject track` is not
    reproducible (2026-10-28, no issue): no allocation is left in the
    original or the port after small programs of every construct,
    generated programs of 4000 lines, the largest CD programs (among them
    `GORNY3DLIB.BB2`, 127996 bytes) and one CD source of each of the 47
    compile errors of `blitz2-cd.tsv`), #26 (needs a free block of
    ≥ 256 MB).
  - *Done 2026-10-29 by five agents in parallel* (worktrees, integrated
    on main): residents (tedstub `resident=`, inject `pad`, sources as
    test data `AREA/NAME.src`: #34, #35, #36); Compile & Run / PANIC
    (tedstub `run=`, inject `createprocfail`: #24, #25); input (inject
    `click=`, `doslocked`, `keys=`, `opencount=`, `cd`, `inuse=`: #100,
    #154, #156, #157, #158); the serial remote end for bug-orig tests
    (replies `N`, `S`: #252, #256, #258, #259); the cause of #335 (a
    compiler bug, test added) and with it #90, #91. Found there: #338
    (port: runtheprog did not load a4 with -fbaserel, Compile & Run never
    started a program; fixed, the binary changed).
  - *Solved:* `ted_ab` case 01d-rt-maplib timed out in R5 after 16
    runs in one session (#339). Cause: the original bug #340 - TED's
    a0toname splits the file argument by blitz2's LoadSeg result in d0;
    with the low byte '/' TED opened `mikesmaplib.b/b` ("Couldn't open
    file!"). It depends on the memory layout, so any run can hit it (seen
    also: 05-find C5). Found with `AMIGA_EMULATION_SHOTS` and a temporary
    log of failing DOS `Open` calls. teddrive now recognises it and tedab
    repeats such runs in a new session (76ea411); teddrive also sees a
    window opened at the address of a closed one as new (66fa3db).
    Orphaned Amiberry processes of a crashed runner: fixed with
    PR_SET_PDEATHSIG (1651778).
  - *Deferred (user decision 2026-10-29):* removing a patch is racy.
    teddrive's CloseWindow patch calls the original with `jsr`: a task
    still inside it when teddrive removes the patch and ends returns into
    freed code; the Delay of 0.2 s only narrows that. Also with a `jmp`
    patch (teddrive's Open, reqtrap's AutoRequest/EasyRequestArgs) a task
    interrupted between the library vector and the stub runs into freed
    code. Fix: stub and its data in an AllocMem block that is never freed
    (about 100 bytes per run), teddrive reads the data through a pointer.
  - *Runner features:* done: program arguments (`runargs=`, #107),
    the macro buffer of the default options (tedstub `macrobuf=`, used
    for #336), the start from the Workbench (`wbrun`: #19, #20, #306,
    #307, #21: the port ends in the start directory, libnix's startup
    restores it - a deviation, documented (user decision 2026-10-29); from
    the CLI blitz2 does not change the directory, correctly); found
    there: #337 (wb_exit: Permit before blitz2 has returned; test with
    wbrun freeseg, the port comes out FIXED, libnix answers without
    Permit); a program needs the command WBStartup to run from the
    Workbench (manual), not a bug; lib_OpenCnt of dos.library is no
    measure in the emulator (VNCClip.supervisor opens and closes it in
    the background), wbrun counts the calls of the process; the
    others see above.
  - *RWE userlibs* (about 180 issues, labels `lib:ri…`, `ciatrklib`,
    `bb_blitlib` ...; 2026-10-30): Blitz programs in
    `tests/bug-orig/rwe/ID.bas` with `; @rwe NAME...` (`tests/README.md`, "bug-orig tests"):
    ref = blitz2 2.10 of the CD with the library's reference (`RWE_LIBS`;
    a reference build requires the fixture `rwe_refbuild`), port/clib
    with the C build; the pass file's `file=DEST|SRC|TAG` stages a file
    for one configuration (`corpus_runner`: a tag without configuration
    fails). Pilot, all REPRODUCED and FIXED with a temporary fix: #851
    (wrong result), #827 (64 KB overwrite kept inside the list's own
    memory), #649 (d0 left as the address argument; the serial remote
    end), #680 (stack: crash, the session ends in a timeout). Open: a
    test of rievallib (variant `rwe-1999-eval`, number 121 = polylib of
    the CD's acidlibs) needs a ref with other acidlibs.
  - *Not reproducible in the emulator:* #42 (a real 68010 with a moved
    VBR; Amiberry runs a 68020).
  - *Found during phase 2:* #333 (palettelib chunk walk: the port
    compares the sign of the difference, the original the addresses;
    label `bug-port`, with an audit of the same pattern in 2dlib,
    polylib, linklistlib), #334 (medlib: a masked track does not
    advance the note pointer, original bug, test done), #335
    (programs using blitzoslib hang at End: a compiler bug, calcmaxs
    before fixjsrs, test done).
- [ ] **Emulator tests that fail under load near their time limits**
  (observed 2026-11-05, several agents using the emulator at once): the
  session time limits are fixed numbers of seconds that count from the
  start of the Amiberry process, so a slow machine eats the margin
  (`amiga_emulation.h:46-57`: the wait for a slot does not count; a
  first-save session's cold boot until the save has a limit of the same
  length). Observed:
  * `regression_ricompactdisclib_scsi_after_plain`: ref session 61.8 s
    against the 60 s of its `; @pass timeout=60`
    (`tests/bug-orig/regression/ricompactdisclib-scsi-after-plain.bas:13`)
    for its first-save snapshot; alone and in other runs it passed
    (76.85 s total). The default `BUG_ORIG_TIMEOUT` is 90
    (`tests/bug-orig/CMakeLists.txt:73`).
  * `host_emulator` (ctest `CMakeLists.txt:113-116`, 300 s plus the queue
    wait; runs the host tests with the label emulator, among them the
    snapshot test, session `p.timeout = 90`,
    `tools/tests/test_amiga_emulation_snapshot.c:139`): "no free emulator
    slot". `take_slot` (`tools/lib/amiga_emulation/amiga_emulation.c:797-805`)
    fails at once, without waiting, when the process already has a
    session running and every slot is taken (`AMIGA_EMULATION_MAX`,
    default 4).
  * `ted_ab` (`TED_AB_TIMEOUT` 120, `tests/ted-ab/CMakeLists.txt:39`),
    `ab3_bb2_equivalence_runerr` (`AB3_EQUIVALENCE_TIMEOUT` 600,
    `tests/ab3/CMakeLists.txt:30`), `corpus` (`CORPUS_TIMEOUT` 300,
    `tests/corpus/CMakeLists.txt:53`; the output of
    `tests/corpus/tests/t73_menus.bas`): failed occasionally, passed when
    rerun alone. The cause was not analysed; the limit in question
    (session, ctest `TIMEOUT` or the slot) is not recorded for these.
  The ctest `TIMEOUT` of the tests already includes
  `EMULATION_QUEUE_WAIT` (7200 s, `cmake/emulation-queue.cmake`) per
  session; only the session limits (`timeout=` of the runners, `@pass
  timeout=`, `AMIGA_RUN --timeout`) and the fixed `host_emulator` /
  `amiga_emulation_*` limits (`tools/tests/CMakeLists.txt:51-62`) have no
  allowance for load. To decide: (a) raise the limits (the session
  limits of tests close to them; hangs are then caught later), (b) scale
  them with the load (e.g. by the number of busy slots or the measured
  boot time of the session), or (c) serialise the tests that run
  near their limits (ctest `RESOURCE_LOCK` or a lower `AMIGA_EMULATION_MAX`
  for the agents). First step: record which limit ended each failure
  (the `@stat` line has `end=timeout`, `tests/README.md` "Test statistics").
- [ ] **rwe_residents_cd compares bytes**: since #970 Create Resident of
  blitz2-c writes 0 into the bytes the loaders do not read (pad bytes,
  a constant's data and procnum, pointer remains), so its residents are
  reproducible; `tools/blitzres/check.cmake` still compares the dumps
  (`blitzres --dump`), because the original's files differ in those
  bytes. Measured (2026-11-05, `lvo`, `custom`, `datatypes`, `AHI`):
  the port's files equal blitzres's byte for byte except the long word
  padding at the end of the hunk, which menu5 fills from `hello`
  (`'ZAP',0`, stmts.c) and blitzres with zeros (`lvo`, `custom`,
  `datatypes`: 2 bytes each; `AHI` is equal). To do: blitzres pads with
  `'ZAP',0` like menu5 (changes the residents of the build:
  `libs/userlibs/rwe/residents/`), and `rwe_residents_cd` compares the
  port's files with blitzres's byte for byte (the reference's still by
  dump), with a test on a broken file.
- [ ] **Coverage: open points** (openspec change `test-coverage`,
  `tests/README.md` "Coverage"):
  1. [ ] `genhtml --branch-coverage` refuses `all.info` (without the
     option it reads it): lcov 2.0's consistency check finds host lines
     run with no branch of them taken (gcov data of tests whose processes
     fork and end with `_exit`, e.g. `test_amiga_emulation_run.c:200`),
     and lcov 2.5 fails on its own count check when it merges a branch
     taken 0 times in one test with one taken in another
     (`BranchElement::merge` in `lcovutil.pm` marks no change then).
     Decide whether `covmap` leaves out the branches of such lines, or
     whether the branch data waits for lcov.
  2. [ ] Tests for the 182 measured compiler functions no test reaches
     (most of them compile error entry points and the editor side of
     the compiler); the list comes from the tracefiles of a coverage
     run.
- [ ] **a1200 and a600 on Amiga Forever's Workbench 3.1** (user decision:
  Amiga Forever 11 as the reference set; `AMIGA-EMULATION-SETUP.md`,
  "Setup from Amiga Forever 11"): `cmake/setup-amiga-forever.cmake` makes
  `amiga-os-310-a1200.rom`, `amiga-os-310-a600.rom`, `workbench-311.hdf`
  and `workbench-311-rtg.hdf`, and all three boot and run
  `TEST:run.script`, but the configurations (`default.uae`, `a600.uae`,
  the RTG variant) still name the AmigaOS 3.2 files. With them switched
  to 3.1, `ctest --preset emulator` (2026-11-06) failed 11 of 90 tests,
  10 of them only on 3.1 (passing on the 3.2 files): `ab3_cli`,
  `ab3_cli_ref` and their `_broken` variants, `ab3_tokenise`,
  `ab3_tokenise_broken` (AmiBlitz3 asks for `ab3pft.library`, no
  executable), `libs_emulator` (rimisclib: no `helvetica.font`, font
  prefs; rilesdebuglib OutText; mathtranslib finit registers), `ted_ab`
  (`28-long`: "character needs more than one key (MapANSI)", the
  `29-cmp-*` runs skipped), `corpus_cd`/`corpus_cd_ab3` (6 executables
  of `LINE_TORUS*.BB2` differ), `regression_ricompactdisclib_scsi_after_plain`
  (timeout of the reference run). Needed: decide which software the 3.1
  reference gets on top (AmiBlitz3's libraries, fonts, further
  libraries; added by the setup with `hdftool`, from a source with a
  clear licence) or which tests depend on the 3.2 hardfile's software,
  then switch the configurations and the Kickstart version checks of
  the `amiga_emulation` tests (3.2 = 47 to 3.1 = 40).
- [ ] **Immutable strings (idea, long term)**: in BB2 a string variable
  is string and string builder in one (the memory of the variable is
  changed in place: `MaxLen`, `&a$` with `Poke`, libraries writing the
  length at -4). Immutable, shared strings as in .NET would need
  reference counting (no garbage collector), a separate builder type,
  constant strings in the code or data hunk (the hunk format has no
  read-only type) and libraries written for them. Not for BB2 or AB3
  mode (existing sources and AmiBlitz3's binary libraries rely on
  mutable strings): only as a language variant of its own.

## 3 Only once §1 is done

- [x] **C23** done (2026-10-29): `gnu23` instead of `gnu11` on both
  sides, `static_assert` instead of `_Static_assert`, `constexpr` for
  the constant scalars (`static const` with a fixed value, constant
  local lengths). The `#define` constants stay (mostly mirrors of `equ`,
  used in `#if` and asm strings); the `enum`s are real enumerations.
  All 4732 hunk files of the build byte-identical, `ctest --preset all`
  green.
- [x] **Comparison with blitz2 2.10** done (2026-10-29): version 210,
  the `/` test (#2 fixed), the `Select` templates as Devpac encodes them;
  the compiler's output is compared with `BLITZ2/BLITZ2` of the CD
  (corpus, `corpus_cd`, `ted_ab`; `tests/data/blitz2-cd.tsv`
  regenerated), the bug-orig tests keep the 1.95 build; new corpus test
  `t14_select` (`docs/v2.10/README.md` §4). TED, acidlibs and deflibs on
  the CD are the ported state. With this the port is complete: original
  bugs may now be fixed (`CLAUDE.md`, "Do not fix original bugs"); the
  basis are the issues with the label `bug-orig`.
- [ ] **blitz2 and TED on the shared `.xtra` code** (`shared/xtra/`,
  2026-10-30, reworked the same day; `docs/formats.md` §1.4, §7): an
  intended deviation of blitz2's binary, checked functionally. TED (or
  blitz2's `loadxtra_c`) reads the source and `NAME.xtra`
  (`xtra_companion_name`) into memory, `xtra_load` (the type from
  `xtra_detect`); the caller keeps the `.xtra` buffer and the source
  header (`x->hdr` bytes) while it uses the views. `loadxtra_c` then
  copies into the gadgets (`icon` → `optreqga9`, `sw[]` →
  `optreqga10–16`/`optreq2ga1–7`, `buf[]` → `optreqsg1–6`, residents
  (`xtra_text`) → `resbuff` with `seg = 0`, exe path/name → `xpathname`,
  CLI argument → `argreqla8`, the maxima (iterator) →
  `findlibnoerr`/`LIBMAX_COUNT`, vers 0 → `xtra_apply_vers0`; keep the
  `ezerr` `BUG(orig)`); `savextra_c` the other way round
  (`xtra_set_text` on the gadgets' buffers, `res_ptr[i] =
  resbuff[i].seg`, `xtra_set_maxima`, `xtra_write_blitz2` into a buffer
  of `xtra_blitz2_size` bytes, one `Write`). `struct xtra` is 300 bytes
  on the m68k: it need not be a global of blitz2 (64 KB `-fbaserel`
  data), a local or allocated one will do. Open: blitz2's loader accepts
  short files, other sizes and vers > 1, which `xtra_load` refuses – a
  lenient mode in the core or a fallback; an `XTRA_DIR` like
  `FILEREQ_DIR`. TED 1.1 only calls blitz2's callbacks; with BB22 or
  `.ab3` sources it would take the cursor from the loaded header and
  keep the header bytes for saving (`xtra_write_ab3` updates them).
  `xtra convert` drops maxima between AB3 object names and library
  numbers (needs the libraries' object names).

## 4 The libraries of Red When Excited

Decision of 2026-10-29: the library sources RWE released in September
1999 (`archive/aminet/amibl2src/rwelibsrc/`, from `RWELibSourc.lha`;
origin and license: `archive/README.md`) are ported – as libraries of
their own for `blitz2:userlibs/<dir>/` (works since #2), next to the 2.10
state; `acidlibs`/`deflibs` stay byte-identical. About 45 libraries
(Reflective Images, Leading Edge Software, RWE and a few others), many in
several versions.

1. [x] **Inventory** done (2026-10-29): `docs/rwelibs/README.md` –
   about 50 libraries, 22 with a source version whose declarations
   equal a binary, about 25 only as source; `ROBNORTHFAST` not ported
   (license). Was: per library the source files and
   versions, author, library number, tokens; the matching binaries of the
   CD (`EXTRAS/BLITZLIBS/RILIBS`, `BSS2/REDBLITZLIBS`, `…/OTHERLIBS`,
   `docs/v2.10/libs.md` §3) and from that the version that is the
   reference; number clashes with acidlibs/deflibs.
   AmiBlitz3 (SourceForge `amiblitz3`, git of the SVN) helps (checked
   2026-10-29): `Sourcecodes/Includes/System/sorted_libnums.ab3` names
   number and often author of every RI/RWE library (e.g. `#Risortlib`
   = 13, `#RILVCallHook` = 27; clashes noted there, e.g. 35, 56, 74);
   `BlitzLibs/UserLibs/userlibs/` has 32 RI/RWE binaries, never changed
   in its history (all from commit `8a56975`, 2012), 27 of them
   byte-identical to `BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS/` (and
   others) of the CD; only in AmiBlitz3: RIAmosFuncLib, RIDisAsm,
   RILESDEBUGLib, RILVCallHook, Rimisclib. `Docs/UserLibs/RI*.guide`
   (30 manuals) and `Docs/UserLibs/Examples/RI*` (example programs) for
   the behaviour and the tests. No library sources of RI/RWE in its
   history.
2. [x] **Tokens** (done with 5, token lists from library files): 23 of the 123 sources use tokens the list of blitz2 1.95
   (`tests/data/ted-tokens.txt`) does not know – the token list for
   reading them (teddetok, a readable `rwelibsrc` like `bblibsrc`). The
   token tables are in the binaries (CD, for the five others AmiBlitz3);
   AmiBlitz3's `BlitzLibs/tokenlist.txt` (`NAME.obj(NUM):INDEX token`)
   lists them as well, for comparison.
3. [x] **Reference** (done: the reference build `rwe_refbuild`, 5.5): how the libraries were built from the Blitz sources
   (`libmacs.bb`, `!libheader`), and whether a reference build is
   possible here; otherwise the CD binaries.
4. [x] **Port** (done 2026-10-30: all libraries of `RWE_LIBS` are C, 5.5) per library to C (`cmake/BlitzLibrary.cmake`), with the
   checks of the acidlibs (`blitzlibcheck`, declarations, `asmcmp`,
   emulator tests); output `build/<preset>/blitz2/userlibs/…`.
5. [ ] **Library variants and token lists from library files** (decision
   of 2026-10-29). Library numbers clash between libraries of different
   authors (7: RI `ArraysLib` and Oliver Michel's `OM3D`; 195 and 48
   in the history of blitz_ptplayer), so the libraries are built in
   variants, and the tokens of a source come from the library files it
   was written for, not from one fixed list:
   1. [x] **Token list from library files** done (2026-10-29):
      `tools/blitztokens` (logic `tools/lib/blitztokens`) makes the list
      as blitz2 builds it for the editor. The BASIC keywords and
      assembler mnemonics are not in library format: blitz2 keeps them
      as a linked list of nodes (`blitz2.s` 283 `tokens`, next.l num.w
      name help) in its own file; the tool finds the node of token 1
      `NEWTYPE` and follows the relocated `next` longs. Then the
      libraries in the compiler's order (`loadlibs`: acidlibs, deflibs,
      userlibs) numbered like `fixlibs` (user library libnum*128+1,
      system library from libnum, every entry counts, a nullsub has no
      token). Equal to `tests/data/ted-tokens.txt` (1821 tokens, the
      same order, so also the same choice for a name defined twice) for
      the CD's 2.10 compiler, the reference build of `blitz2.s` 1.95 and
      the port, each with the CD's acidlibs/deflibs, and for the port
      with the libraries built here (ctest `tokens_cd`, `tokens_port`,
      `tokens_broken`; unit test `blitztokens`).
   2. [x] **`tedtok`/`teddetok` with library files** done
      (2026-10-29): besides `TOKENS` (a list file, as before) the form
      `--blitz2 BLITZ2 [--lib LIB]...` makes the list from the files
      (`programs/ted/tests/host/tokref.c` on `tools/lib/blitztokens`);
      `blitztokens` exports it as a list file for both. The
      readable `rwelibsrc` from the files of its variant: done
      (2026-10-30): `blitz2`, the variant's acidlibs/deflibs and all
      RWE userlibs built here (for rievallib its own variant), no CD
      binaries (the USES field of `RWE_LIBS` is gone); all 52 files
      identical to those read with the CD binaries.
   3. [x] **Numbers only through `libnums.h`** done (2026-10-29):
      `debuglib.c` writes its four tokens in inline assembler with
      `BL_ASMTOK(LIB_inputoutputlib, 4)`/`(LIB_printlib, 1)`
      (`blitzlib.h`); the numbers libraries gave literally or defined
      themselves are at the end of `libnums.h`: `LIB_fadelib_header`
      (157; `#fadelib` of libnums.bb is 102 and used by no library),
      `LIB_polylib` (121, `#chunkylib` in its source), and of deflibs
      `LIB_colorwheellib` (219), `LIB_mathtransoslib` (225),
      `LIB_xprzmodemlib` (2); all 994 results byte-identical. No other
      literal or local number in the libraries (`BLITZ_LIBHEADER`,
      `BLITZ_LIBS`, tokens of ALibJsr/BLibJsr all through `LIB_xxx`
      of `libnums.h`).
   4. [x] **Variant build** done (2026-10-29): `libs/variants/NAME/`
      with its own `libnums.h` (it may include another one and change
      single numbers); `libs/variants/bb2-2.10` (the former
      `libs/include/libnums.h`) is the default variant
      (`BLITZ_DEFAULT_VARIANT`), built to `blitz2/`; `acidlibs_build`/
      `deflibs_build` build the same sources with the variant's
      directory first in the include path (`blitz_library … VARIANT`,
      `blitzlib.h` includes `<libnums.h>`), the other variants to
      `build/<preset>/variants/NAME/acidlibs`, `deflibs`. The default
      build is byte-identical. Check in the build: `biglib check` in the
      join (no number twice in acidlibs, in deflibs with acidlibs),
      tests `biglib_check_clash` (must fail) and the unit test.
      Test variants (`libs/tests/variants/`): `selftest-same`
      (byte-identical to bb2-2.10), `selftest-renumbered` (fadelib 157→3, inputoutputlib
      164→4: the tokens move, debuglib's call of inputoutputlib with
      them; `variant_selftest_renumbered`). Userlibs of a variant: done with 4.5.5
      (`RWE_OWN_VARIANTS`). Further switches besides the numbers: none
      needed (decision 2026-10-30), added only when a library needs one.
   5. [ ] **First users**: variant `rwe-1999`, userlib build
      `libs/userlibs/rwe/` with the list `RWE_LIBS`, target `rwelibsrc`.
      Stage A done (2026-10-29): the 15 libraries with a byte-identical
      reference build are real C (decision 2026-10-29: declarations equal
      to the CD binary, behaviour compared with it in Amiberry,
      `libs/tests/rwe/NAME.c`, 22 test cases of `libs_emulator`; notes per
      library in `docs/rwelibs/port/`). Stage B with a binary as the
      reference: AMOS functions (the AmiBlitz3 SVN binary), Sound,
      StoneCracker and FNS (BSS2) done (2026-10-29); the reference build
      of the sources is part of the tests (`rwe_refbuild`, REFERENCE
      `refbuild`). The stage B/C libraries whose reference is the source
      (§3.2) are in `RWE_LIBS` (2026-10-29): 13 with REFERENCE
      `refbuild`, all compile in `rwe_refbuild` (BB22 sources through
      `tools/bb22`); Anim with the BSS2 binary (its source does not
      compile); LES debug not yet (`Font.bin` is missing). C ports done
      (2026-10-30), CIA tracker and ptlib (`ritrackerlib`, 58, ptlib6)
      included (the Protracker 2.3A play routine, released as "Public
      Domain"; decision 2026-10-30, `archive/README.md`). LES debug
      (`Font.bin` cut from AmiBlitz3's binary), Eval (121, variant
      `rwe-1999-eval`, polylib at 87) and stage D (§3.3, 16 libraries
      without a binary, REFERENCE `refbuild`) ported (2026-10-30): all
      52 libraries of `RWE_LIBS` are C (`docs/rwelibs/README.md` 3.1
      "Ported"); then risoundlib2 (Sound with AHI, variant `rwe-1999-ahi`
      instead of risoundlib, 2026-10-30): 53. Every `BUG(orig)` of the
      RWE libraries has its GitHub issue (#344–#922, risoundlib2's #924–#934,
      2026-10-30; the numbers in the code, port notes and tests). **Reference build in the build** done (2026-10-29,
      `libs/userlibs/rwe/refbuild.cmake`): residents by `tools/blitzres`
      from the NDK (contents equal to blitz2's, `rwe_residents_cd`),
      staging (target `rwe_refstage`; `INCDIR` neutralised on the bytes
      by `tools/bblines`, `LIBNUMS.BB` with the numbers of `libnums.h`),
      ctest `rwe_refbuild` (cdrun `source=`, Amiberry, fixture) to
      `build/<preset>/libs/userlibs/rwe/ref/NAME.obj`; stage A byte for
      byte equal to its binaries (`rwe_refbuild_NAME`). For the
      libraries whose reference is the source (§3.2 "Source", §3.3):
      REFERENCE `refbuild` in `RWE_LIBS`. `AHI.res` of risoundlib2
      done (2026-10-30): constants only, from the AHI SDK of the
      toolchain (`rwe_residents_cd`). Open: the further includes of the
      sources not yet buildable (`docs/rwelibs/README.md` §3.1). The RWE userlibs (their numbers: AmiBlitz3's
      libnums, the matching binaries, `#riarrayslib` = 7 from
      `defaultdbug50.bb2`; `docs/rwelibs/README.md` §3.1). ptlib is
      ported after all (decision 2026-10-30, `archive/README.md`); no
      replacement around Frank Wille's ptplayer (decision 2026-10-29).
