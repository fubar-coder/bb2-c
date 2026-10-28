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

* **`-h` / `--help` for all callable programs** (request of
  2026-10-28): usage text to stdout, return 0; today none of them knows
  it. Affected: the host tools `tools/*` (amiga_run, asmcmp, biglib,
  blitzlibcheck, blitzlibcompare, checkdata, checklabels, hunkcheck,
  vasmlisting), `programs/ted/tools/tedtok`, `tests/corpus/runner`
  (corpusrun, cdrun) and the Amiga programs `reqtrap`, `reqclose`,
  `reqclosetest`. Not included: `tedstub` and `tedoracle` (only blitz2
  loads them as `blitz2:ted`; started from the shell they end with 20).
  Also excluded (decision of 2026-10-28, may change much later):
  `blitz2` and `ted` themselves – they reproduce the behaviour of the
  originals (argument = file), a help would be a `DIFF`.

* **Git history and remote:** this repository is the reference
  (decided); open whether it stays with the fresh start or takes over the
  history of the files from bb2, and where it is pushed to.
* **Port `dhandlerlib` (lib 1):** until then `prebuilt/dhandlerlib.obj`
  (original of the CD).
* **Warnings of `blitzlibcheck`:** 73 warnings (references into the error
  check area, as with `mkblitzlib.py`) on every rebuild of the affected
  libraries – if needed, behind an option.
* **LTO for TED** as a separate experiment: small benefit (one call across
  the boundary to `filereq_core.c`, 12 bytes); risks `glue_start.c` at the
  start of the hunk (`hunkcheck` checks that), `-fbaserel`, glue with
  inline assembler. Not for the libraries (`-fno-toplevel-reorder`).
  ab3c uses LTO for its Amiga build (`AB3C_LTO`) – look at the experience
  there.

* **License header lines** (SPDX identifier `GPL-3.0-or-later WITH
  LicenseRef-BlitzBasic2-Runtime-exception` or similar) in the source
  files – not set yet; the license applies via `LICENSE` and the README.
* ~~**CD data**~~ solved (2026-10-28): not in the repository,
  `cmake/fetch-originals.cmake` loads the CD (archive.org) and the Aminet
  archive with a fixed SHA-256 to `archive/cd/`, `archive/aminet/`.
* ~~**Documentation `docs/`**~~ done (2026-10-28): paths, commands and
  line references (C code: determined anew by content) adapted to this
  repository; history of bb2 (plans, logs, reviews) unchanged in content,
  remaining bb2 mentions explained in `docs/README.md`.
* **Tests for the issues labelled `needs-test`** (99 open issues; for
  most a test program that triggers the bug is missing, the trigger is
  described in the issue). A failing test is added with label `bug-orig`
  (README, "Tests") and named in the issue. What is needed:
  - *Test programs only* (no example, no corpus test yet) – compiler:
    #2, #3, #5, #8, #14, #16, #18, #22, #23, #26 (needs a free block of
    ≥ 256 MB), #27, #28, #32; libraries: #58, #65, #90, #91, #100,
    #104, #110, #115–#118, #127, #133, #135, #137, #171, #177, #191,
    #224, #245, #246, #263, #264, #266, #268, #269, #274, #280, #281,
    #286, #299, #303, #304, #308, #310, #314, #316, #318, #319, #321,
    #322; medlib/trackerlib without own data: #204–#207, #289–#291,
    #293–#295 (need a module that uses the effect).
  - *Test data* (generated or freely licensed, under `tests/data/`):
    MED modules – any (#212), with 8 tracks (#209), several tracks for
    SetMedMask gaps (#208), 19xx/0Bxx in one line (#203), finetune
    outside −8..7 (#211); 8SVX – BODY ≥ 131072 bytes (#67), BODY length
    a multiple of the DiskBuffer length (#69), BODY length 0 (#179);
    ILBM – 5 planes in memory (#80), odd-sized text chunk before the
    BODY (#189) or before CMAP/CRNG in memory (#225).
  - *Runner features:* other compiler options (buffer sizes; the corpus
    stub sets fixed ones) for #4, #57; creating residents for #34, #35,
    #36; start with program arguments in the corpus runner (#107,
    covered at library level by `bug_orig_cliargs_parpath_cli`); start
    from the Workbench (blitz2: #19, #20, #21; programs: #306, #307);
    Compile & Run / PANIC from the editor (#24, #25); GUI input in the
    file requester (#154, #156–#158); a serial peer for unit > 0 and
    pending writes (#252, #256, #258, #259).
  - *Not reproducible in the emulator:* #42 (a real 68010 with a moved
    VBR; Amiberry runs a 68020).
  - Done: #112, #119 (`bug_orig_crunch_implode_overread`,
    `bug_orig_crunch_decrunch_corrupt`).

## 3 Only once §1 is done

* **C23** (`gnu23` instead of `gnu11`): `static_assert` instead of
  `_Static_assert`, `constexpr` for constants. Acceptance: all products
  byte-identical, all checks from §1 green.
* **Comparison with blitz2 2.10** (the binary of the CD): completion of
  the port. Only after that may original bugs be fixed
  (`CLAUDE.md`, "Do not fix original bugs"); the basis are the
  issues with the label `bug-orig`.
