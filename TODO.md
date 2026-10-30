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

1. ~~**Reference build of `ted.s`**~~ done (2026-10-27): target
   `ted_ref`, original sources in `archive/sources/ted-1.1/`,
   `tools/vasmlisting` instead of `tedlisting.py` (`map`, `labels`,
   `verify`); `ref/ted`, listing, map and labels byte-identical to bb2;
   `verify` is a check in the build step.
2. ~~**`checkdata`**~~ done (2026-10-28): `tools/checkdata` (instead of
   bb2 `tools/checkdata.py`, output identical line by line), table
   `programs/ted/checkdata.tsv` (534 entries), POST_BUILD of `ted.sym`
   (second link with symbols as in bb2); `ted` is linked only after that.
   A missing C symbol is now an error (in bb2 "todo").
3. ~~**`checklabels`**~~ done (2026-10-28): `tools/checklabels`
   (instead of bb2 `tools/checklabels.py`, output byte-identical) as
   POST_BUILD of `ted`: 278 code labels, all exactly once.
4. **Host tests of the TED core** – done (2026-10-28): all 10 tests of
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
5. **TED A/B tests in the emulator** (bb2: `tools/tedab.py`, `teddump.py`,
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
   1. ~~Amiga tools~~ done (2026-10-28): `tests/ted-ab/amiga/`
      `teddrive`, `teddrivetest`, `tedprobe` (byte-identical to bb2;
      `teddrive` includes `tests/corpus/amiga/reqclose.h`); host test of
      the script parser `tdscript.h` as snitch (`ted_ab_tdscript`).
   2. ~~Dumps on the host~~ done (2026-10-28): `tests/ted-ab/runner/`
      `teddump` (library `lib_teddump` and tool `teddump info|text|cmp`):
      decoding with the fonts from `archive/sources/ted-1.1/`, masking
      "Free", comparing; SHA-256 instead of SHA-1, the deviating lines
      instead of a unified diff; no PNG for now. Test `ted_ab_teddump`
      after bb2's `test_teddump.py`. On bb2's 150 real dumps: text
      byte-identical to `teddump.py`, all 124 pair comparisons judged the
      same.
   3. ~~Runner `tedab` in C~~ done (2026-10-28): `tests/ted-ab/runner/`
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
   4. ~~ctest `ted_ab`~~ (label emulator, fixture on `corpus`; filters
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

6. ~~**Instruction comparison of the assembler routines**~~ done
   (2026-10-28): `tools/asmcmp` (instead of bb2 `tools/asmcmp.py`, output
   equal, also for deviations), ctest `asmcmp_runerrlib`,
   `asmcmp_debuglib`, `asmcmp_memacclib` with the routine lists from bb2;
   originals from the CD (`BLITZ2/ACIDLIBS`, equal to bb2 `ref/*.obj`),
   symbols from `NAME.linked` (`HUNK_SYMBOL`, no `nm`).
7. ~~**Declaration comparison with the originals**~~ done
   (2026-10-28): `tools/blitzlibcompare` (rules of bb2 `biglib.py
   compare`, previously by hand), ctest `declarations_acidlibs`,
   `declarations_deflibs` against `BLITZ2/ACIDLIBS`, `DEFLIBS` of the CD:
   83 + 39 libraries, declarations identical, same order (cross-checked
   with `biglib.py compare`: 122 identical).
8. ~~**snitch tests in the emulator**~~ done (2026-10-28): `libs/tests/`
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

9. ~~**Corpus**~~ done (2026-10-28, phases 1-5 below): bb2's
   `tools/corpus_check.py`, `bb2corpus.py`, `bb2compile.py`,
   `serialrun.py`, `corpus/`, `tedstub`, `reqtrap`, `reqclose` are here in
   C/CMake (`tests/corpus/`); `ctest --preset emulator` about 3 minutes.

   **Plan (2026-10-28, agreed):** location `tests/corpus/`
   (cross-cutting).
   1. ~~Amiga tools~~ done (2026-10-28): `tests/corpus/amiga/`
      `tedstub` on `shared/edstub` (batch mode only; a single job is a
      list with one entry), `reqtrap`, `reqclose`, `reqclosetest`
      (byte-identical to bb2). Trial run with blitz2 1.95: results, error
      line and hashes as in bb2 `reference/195.tsv`.
   2. ~~Test sources~~ done (2026-10-28): `tests/corpus/tests*/*.bas`
      (135); tokenized with `tedtok` (C core of TED, `fileopen2` +
      `realsave`, real token list), which rejects text lines over 129
      characters (TED truncates them). 15 such lines in 9 sources from
      bb2 split (`NPrint a,…,z` → `Print a,…` / `NPrint …,z`, same
      output); after that all 135 byte-identical to bb2tok. Instead of
      bb2's `default.xtra` (origin unclear) the stub with `runerr=off`
      writes the startup options of the compiler without error
      checking/debugger.
   3. ~~Check matrix~~ done (2026-10-28): `tests/corpus/runner`
      (`corpusrun`, C on `amiga_emulation`), ctest `corpus`,
      `corpus_runerr`, `corpus_req` (label emulator): **201 checks, 0
      deviations**, about 100 s (3 emulators in parallel; serially about
      three times as long). clib = C acidlibs and C deflibs. The programs
      load sounds and modules from the CD via `BB2:` (as in bb2). Program
      outputs byte-identical to bb2's runs except for `t13_env` (reads
      the environment on purpose: Workbench 640×512×8 instead of
      640×256×2).
   4. ~~CD reference run~~ done (2026-10-28): `tests/corpus/runner/cdrun`,
      target `blitz2-cd-reference` (blitz2 1.95, about 35 s) →
      `tests/data/blitz2-cd.tsv`: 498 compiled, 217 errors, 9 text
      sources; all 724 lines equal to bb2 `reference/195.tsv`,
      reproducible. ctest `corpus_cd` (blitz2-c): 724 equal, about 30 s.
   5. ~~Serial test~~ done (2026-10-28): `tests/corpus/runner/serial.c`
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
  `amiga-emulation/` with README (expected files, checksum of the ROM,
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

* **`__saveds` instead of hand-written a4 loads** (user decision
  2026-10-29): since m68k-amigaos-gcc 16.2-rc21 (AmigaPorts/gcc#141)
  `__saveds` loads a4 again (`jsr ___restore_a4`); older versions accept
  it and generate nothing - wrong code without a warning. Steps:
  1. ~~toolchain check~~ done (e6e3dbb): configure compiles
     `cmake/saveds/good.c` and needs `jsr ___restore_a4`; test
     `toolchain_saveds_broken`; README and CLAUDE.md: gcc ≥ 16.2-rc21
  2. ~~`runtheprog`~~ done (3a572c3): `__saveds` instead of the
     `runtheprog_a4` stub; blitz2 changed (intended), checked with
     `ctest --preset all` and the bug-orig tests with Compile & Run
  3. the other `GLUE_LOAD_A4` places (blitz2 `glue.c`, `glue_direct.c`;
     TED `glue.c`, `glue_start.c`): recommendation, not done - they are asm
     thunks that save all registers with movem and pass the frame to C;
     `__saveds` on the C function would only replace the one `lea`, save
     a4 a second time and call `___restore_a4`; the thunk stays anyway.
     `directdo` is entered from an exception (rte): the explicit `lea`
     is the clearer form there. Open for a user decision.
* ~~**`-h` / `--help` for all callable programs**~~ done (2026-10-29,
  request of 2026-10-28): usage text with a line on what the program
  does, to stdout, return 0. Host (61736be): the tools, tedtok, teddetok,
  corpusrun, cdrun, tedab, teddump - tests `help_*`
  (`cmake/check-help.cmake`). Amiga (2d19438): reqtrap, reqclose,
  reqclosetest, inject, wbrun, teddrive, teddrivetest, tedprobe - test
  `help_amiga` in Amiberry. Not included: `tedstub` and `tedoracle`
  (only blitz2 loads them as `blitz2:ted`), and `blitz2` and `ted`
  themselves (decision of 2026-10-28: they reproduce the originals,
  argument = file; a help would be a `DIFF`).

* ~~**Git history**~~ decided (2026-10-28): this repository is the
  reference and stays with the fresh start; the history of the files is
  not taken over from bb2 (it remains there). Remote: `origin`
  (GitHub `fubar-coder/bb2-c`).
* ~~**Port `dhandlerlib` (lib 1)**~~ done (2026-10-29):
  `libs/acidlibs/dhandlerlib.c`, byte-identical to the CD's library
  (header in C, the rest assembler: process segment template, relocation
  order; `asmcmp_dhandlerlib`); `prebuilt/` removed.
* **Warnings of `blitzlibcheck`:** 73 warnings (references into the error
  check area, as with `mkblitzlib.py`) on every rebuild of the affected
  libraries – if needed, behind an option.
* **LTO for TED** as a separate experiment: small benefit (one call across
  the boundary to `filereq_core.c`, 12 bytes); risks `glue_start.c` at the
  start of the hunk (`hunkcheck` checks that), `-fbaserel`, glue with
  inline assembler. Not for the libraries (`-fno-toplevel-reorder`).
  ab3c uses LTO for its Amiga build (`AB3C_LTO`) – look at the experience
  there.

* ~~**License header lines**~~ decided (2026-10-28): no SPDX lines in the
  source files; the license applies via `LICENSE` and the README.
* ~~**CD data**~~ solved (2026-10-28): not in the repository,
  `cmake/fetch-originals.cmake` loads the CD (archive.org) and the Aminet
  archive with a fixed SHA-256 to `archive/cd/`, `archive/aminet/`.
* ~~**Documentation `docs/`**~~ done (2026-10-28): paths, commands and
  line references (C code: determined anew by content) adapted to this
  repository; history of bb2 (plans, logs, reviews) unchanged in content,
  remaining bb2 mentions explained in `docs/README.md`.
* **Tests for the issues labelled `needs-test`** (99 open issues at the
  start, 3 open on 2026-10-29: #14, #26, #42). A failing test is added with label
  `bug-orig` (README, "Tests"), named in the section "Test" of the issue,
  and `needs-test` is removed (allowed for these issues, 2026-10-28).
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
  - *Still open, test programs:*
    #14 (the "Not Enough Object Memory" path was not reached in the
    batch run, even with a 128 KB executable: to be found out how;
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
  - *Not reproducible in the emulator:* #42 (a real 68010 with a moved
    VBR; Amiberry runs a 68020).
  - *Found during phase 2:* #333 (palettelib chunk walk: the port
    compares the sign of the difference, the original the addresses;
    label `bug-port`, with an audit of the same pattern in 2dlib,
    polylib, linklistlib), #334 (medlib: a masked track does not
    advance the note pointer, original bug, test done), #335
    (programs using blitzoslib hang at End: a compiler bug, calcmaxs
    before fixjsrs, test done).

## 3 Only once §1 is done

* ~~**C23**~~ done (2026-10-29): `gnu23` instead of `gnu11` on both
  sides, `static_assert` instead of `_Static_assert`, `constexpr` for
  the constant scalars (`static const` with a fixed value, constant
  local lengths). The `#define` constants stay (mostly mirrors of `equ`,
  used in `#if` and asm strings); the `enum`s are real enumerations.
  All 4732 hunk files of the build byte-identical, `ctest --preset all`
  green.
* ~~**Comparison with blitz2 2.10**~~ done (2026-10-29): version 210,
  the `/` test (#2 fixed), the `Select` templates as Devpac encodes them;
  the compiler's output is compared with `BLITZ2/BLITZ2` of the CD
  (corpus, `corpus_cd`, `ted_ab`; `tests/data/blitz2-cd.tsv`
  regenerated), the bug-orig tests keep the 1.95 build; new corpus test
  `t14_select` (`docs/v2.10/README.md` §4). TED, acidlibs and deflibs on
  the CD are the ported state. With this the port is complete: original
  bugs may now be fixed (`CLAUDE.md`, "Do not fix original bugs"); the
  basis are the issues with the label `bug-orig`.

## 4 The libraries of Red When Excited

Decision of 2026-10-29: the library sources RWE released in September
1999 (`archive/aminet/amibl2src/rwelibsrc/`, from `RWELibSourc.lha`;
origin and license: `archive/README.md`) are ported – as libraries of
their own for `blitz2:userlibs/<dir>/` (works since #2), next to the 2.10
state; `acidlibs`/`deflibs` stay byte-identical. About 45 libraries
(Reflective Images, Leading Edge Software, RWE and a few others), many in
several versions.

1. ~~**Inventory**~~ done (2026-10-29): `docs/rwelibs/README.md` –
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
2. **Tokens**: 23 of the 123 sources use tokens the list of blitz2 1.95
   (`tests/data/ted-tokens.txt`) does not know – the token list for
   reading them (teddetok, a readable `rwelibsrc` like `bblibsrc`). The
   token tables are in the binaries (CD, for the five others AmiBlitz3);
   AmiBlitz3's `BlitzLibs/tokenlist.txt` (`NAME.obj(NUM):INDEX token`)
   lists them as well, for comparison.
3. **Reference**: how the libraries were built from the Blitz sources
   (`libmacs.bb`, `!libheader`), and whether a reference build is
   possible here; otherwise the CD binaries.
4. **Port** per library to C (`cmake/BlitzLibrary.cmake`), with the
   checks of the acidlibs (`blitzlibcheck`, declarations, `asmcmp`,
   emulator tests); output `build/<preset>/blitz2/userlibs/…`.
5. **Library variants and token lists from library files** (decision
   of 2026-10-29). Library numbers clash between libraries of different
   authors (7: RI `ArraysLib` and Oliver Michel's `OM3D`; 195 and 48
   in the history of blitz_ptplayer), so the libraries are built in
   variants, and the tokens of a source come from the library files it
   was written for, not from one fixed list:
   1. ~~**Token list from library files**~~ done (2026-10-29):
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
   2. ~~**`tedtok`/`teddetok` with library files**~~ done
      (2026-10-29): besides `TOKENS` (a list file, as before) the form
      `--blitz2 BLITZ2 [--lib LIB]...` makes the list from the files
      (`programs/ted/tests/host/tokref.c` on `tools/lib/blitztokens`);
      `blitztokens` exports it as a list file for both. Open: the
      readable `rwelibsrc` from the files of its variant (once the RWE
      userlibs are built, 4.5.5) instead of the provisional
      `tokens-ext.txt` of `.cache/rwelibs-analysis/`.
   3. ~~**Numbers only through `libnums.h`**~~ done (2026-10-29):
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
   4. ~~**Variant build**~~ done (2026-10-29): `libs/variants/NAME/`
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
      them; `variant_selftest_renumbered`). Open: userlibs of a variant
      (with 4.5.5); further switches besides the numbers when a library
      needs them.
   5. **First users**: variant `rwe-1999`, userlib build
      `libs/userlibs/rwe/` with the list `RWE_LIBS`, target `rwelibsrc`.
      ~~Stage A~~ done (2026-10-29): the 15 libraries with a byte-identical
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
      Domain"; decision 2026-10-30, `archive/README.md`). Next: stage D (§3.3), with
      REFERENCE `refbuild`; Eval
      (121, clashes with polylib) needs a variant of its own. ~~**Reference build in the build**~~ done (2026-10-29,
      `libs/userlibs/rwe/refbuild.cmake`): residents by `tools/blitzres`
      from the NDK (contents equal to blitz2's, `rwe_resident_NAME`),
      staging (target `rwe_refstage`; `INCDIR` neutralised on the bytes
      by `tools/bblines`, `LIBNUMS.BB` with the numbers of `libnums.h`),
      ctest `rwe_refbuild` (cdrun `source=`, Amiberry, fixture) to
      `build/<preset>/libs/userlibs/rwe/ref/NAME.obj`; stage A byte for
      byte equal to its binaries (`rwe_refbuild_NAME`). For the
      libraries whose reference is the source (§3.2 "Source", §3.3):
      REFERENCE `refbuild` in `RWE_LIBS`. Open: residents other than
      constants (`AHI.res` of risoundlib2), the further includes of the
      sources not yet buildable (README §3.1). The RWE userlibs (their numbers: AmiBlitz3's
      libnums, the matching binaries, `#riarrayslib` = 7 from
      `defaultdbug50.bb2`; `docs/rwelibs/README.md` §3.1). ptlib is
      ported after all (decision 2026-10-30, `archive/README.md`); no
      replacement around Frank Wille's ptplayer (decision 2026-10-29).
