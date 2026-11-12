# Proposal

## Why

The test programs of `libs/tests/` load the original library and the C
build side by side and compare them in one program: `SAME(what, expr)`
(`libs/tests/rwe/rwecmp.h:127`) evaluates `expr` with `cur` = 0 and 1,
then passes both values through `tape_obs`. To run without the CD,
`libs_emulator` loads the C build in place of the original too
(`libs/tests/emulator.cmake`, mode `replay`), so every comparison that
does not go through `tape_*` compares the C build with itself and passes
without saying so (`tests/README.md` "Libraries": "compares the C build
with itself in `libs_emulator`"). The tapes (`libs/tests/tapes/*.tape`, 61
files, read and written by `libs/tests/tape.c`, 543 lines) hide the values
behind segment hashes and a kept-register prediction, so a failure names a
range of observation numbers and the text of the first check, not the
value that differs. Both properties follow from running two libraries in
one program.

The other suites (blitz2 corpus, bug-orig, TED) already keep expected data
in the repository, but each has its own form and its own words
(`tests/corpus/notes.tsv`, `.orig` files, `; @fixed #N`, hash tables). This
change also states one model for all of them and implements it for the
library tests.

## What Changes

### One shared test model (new capability `testing/test-model`)

A test is a list of cases (like an xUnit Theory). A case is a key, an
input and an expected output; the verdict per case is the diff of the
actual output against the expected one, and every case is reported by its
key. Inputs are data where they are values (e.g. the opcode words of
`ridisasmlib`), code or an input file where they are sequences (system and
hardware libraries, TED scripts, sources). The test definition holds the
expected output:

- A case on which the original and the port agree is ONE case with an
  unqualified key, run against both.
- A fixed original bug is TWO cases: `KEY [orig]` is the original's
  behaviour, run only against the original (label `originals`); `KEY
  [fixed #N]` is the port's behaviour after the fix, run only against the
  port (every run of `all`). Its value comes from an update from the port
  (reviewed) or is written by hand in the same format. No notes or
  exception table; staleness needs no rule of its own (a port that regresses
  fails `[fixed #N]`, a changed original fails `[orig]`). TED already
  splits its fixed tests this way (task 9.9 of the archived change
  `2026-11-12-expected-results-testing`: Cc in `all`, C5 in `originals`).
- Terms: to *record* is to write expected data from the original, to
  *update* is to write it from the port. The expectation is called
  "expected data" or "recorded original", never "the reference" (`ref`
  means the original program).
- Record and test are separate: build targets with hyphenated names write
  (and are reviewed by `git diff`); tests only compare.

This change implements the model for the library tests only. Follow-up
changes, not part of this one, bring bug-orig (compare-only recording
tests, a recording target that accepts agreeing runs) and the blitz2
corpus (`notes.tsv` rows become `[orig]`/`[fixed #N]` cases) to the same
model; their specs keep their behaviour here.

### Library tests (new capability `testing/library-capture`)

| Operation | Library set | Name | Result |
|---|---|---|---|
| Record | original | build target `libs-capture` (on purpose, not a ctest test; needs the CD) | the expected data `libs/tests/expected/NAME.txt`, written only if two runs with different memory layouts agree, else "test not deterministic" and nothing written |
| Update | C build | build target `libs-update` (no CD) | `[fixed #N]` cases rewritten from the port (reviewed by `git diff`) |
| Test | C build | ctest `libs_emulator` (label `emulator`; preset `all`; no CD) | the port's output equals the expected data (`[fixed #N]` cases applied, `[orig]` not run) |
| Test | original | ctest `libs_emulator_originals` (label `originals`; needs the CD) | the original's output equals the expected data (`[orig]` applied, `[fixed #N]` not run) |

- Both tests keep their names, only compare, and use the same judge. There
  is no separate "capture with the C build": `libs_emulator` leaves the
  port's output in its work directory for diffing.
- The judge (compare, accept, update) is a C tool in `tools/lib`
  (`tools/results libs-compare`, `libs-accept`, `libs-update`) with host
  tests and tests with broken input; CMake only orchestrates (a CMake
  compare of 300 000-line files took 1.4-2.9 s, an indexed loop did not
  finish in 3 minutes, and `file(STRINGS)` splits lines holding an
  unbalanced `[`; measured in the review of 2026-11-12).
- Every test program loads exactly one library set (`NAME.obj` or
  `NAME.orig`, variable `LIBSET`) and writes its observations: one case per
  line, `KEY<TAB>VALUE`. The key is routine, check, vector and occurrence
  number (unique; TAB, `|`, `\` and non-printable bytes escaped). The
  scenario code no longer has the `[2]` arrays, the `cur` loop and the
  second `sets[]` entry.
- Block hashes replace values only for `systest_med` (3.57 million
  observations), `rwe_ridisasmlib` (3.06 million; counts from the `end`
  lines of the tapes) and the two `libtest` programs (`ldivlib`,
  `lmullib`: 41 459 random 32-bit results each, nearly incompressible
  text; design D6). Limits stated in the design: a fix inside a block
  program is a pair of block cases nobody can review, a fix that changes
  the number of observations shifts the block numbers, a program's own
  `check()` that a fix touches must become an observation, and the
  determinism proof covers the memory layout only, not date or time.
- Byte buffers up to 64 bytes are written as hex; larger ones as length
  and FNV hash, with a dump on demand (`LIBS_CAP_DUMP`). Pointers stay
  normalised as the tape does it.
- Kept registers: per routine the test states the set the original keeps
  in all calls; the program writes `kept` or `lost REGS`; equality, no
  subset rule. A register the original keeps only in some calls is not
  covered; accepted.
- The two non-deterministic parts (`riencryptlib`'s random wheel,
  `ciatrklib`'s DMACONR in the F00 phase) are made deterministic where
  possible, else written as `not captured: REASON`.
- Task 1 runs every program several times against the original, before
  any conversion, to list today's non-deterministic checks, and measures
  the committed text size (also compressed) and whether strings and hex
  buffers hold literal CD data. This deliberately reverses the argument
  of task 9.8 of the archived change (13 MB, "a copy of what the original
  computes"); the decision (keep text, or hash more programs) follows the
  measurement.
- `expect_fixed` goes away (the tapes hold no `fixed` line today); the
  pair of cases replaces it.
- Removed: the tape (`tape.c`, `tape.h`, `tapes/`, `tape-compare.cmake`,
  the `LIBSTAPE` modes, the `tape_*` calls, `tape_pause`, `tape_orig`) and
  the replay mode that loads the C build as `NAME.orig`;
  `libs_emulator_tape_wrong` becomes `libs_emulator_wrong`;
  `libs_tapes_compare_*` and `libs_expect_fixed*` go away. The test names
  `libs_emulator` and `libs_emulator_originals` stay.
- **BREAKING** for test authors: the macros of the test programs change
  (`SAME`, `SAMEBYTES`, `DO2`, `cmp_preserve`, `check_fixed`,
  `SAME_FIXED`), and `tests/README.md` "Libraries" is rewritten around the
  model (one paragraph for all suites with the terms above).

Out of scope, a later separate change (setup/act/verify): explicit asserts
for the pure compute libraries, libraries that have no original
(port-only), and checks beyond what the original does.

## Capabilities

### New Capabilities
- `testing/test-model`: the shared model (cases, one case where original
  and port agree, two cases `[orig]`/`[fixed #N]` for a fixed bug, record
  and update, the judge as a tool); implemented here for the library
  tests.
- `testing/library-capture`: the single-library test run, the case format
  and key, qualified cases, record/update/test operations, the block
  programs, the kept-register cases, determinism.

### Modified Capabilities
- `testing/expected-results`: the requirement "Per-check fixed values in
  library pair tests" is removed and replaced by "A fixed library check is
  a pair of cases" (the pair `[orig]`/`[fixed #N]` in the committed
  expected data); the stale scenario names go with it (a rename does not
  pass `openspec validate`, which wants the old scenarios kept).
- `testing/test-selection`: a changed file of `libs/tests/expected/`
  selects both library emulator tests.

## Impact

- `libs/tests/`: `libtest.c`, `systest*.c`, `rwe/*.c` (53 files),
  `rwe/rwecmp.h`, `sysload.h`, `selftest_fixed.c`, `libs_tests.cpp`,
  `emulator.cmake`, `CMakeLists.txt`; new `capture.c`/`capture.h`,
  `expected/` (61 files); removed `tape.c`, `tape.h`, `tape-compare.cmake`,
  `tapes/`. 60 test programs, 61 expected files (`libtest` writes
  `ldivlib` and `lmullib`).
- `tools/lib` (judge), `tools/results` (subcommands), `tools/tests`
  (host tests).
- `libs/tests/bugorig.c` and the `bug_orig_*` library tests use `sets[]`
  with `NO_ORIGINALS`; they keep their form unless the shared header change
  touches them.
- ctest: new tests of the checks with broken input;
  `libs_emulator_tape_wrong`, `libs_tapes_compare_*` and
  `libs_expect_fixed*` go away. New targets `libs-capture`, `libs-update`.
- `tests/README.md` ("Libraries", the presets table), `CLAUDE.md` (names
  the tapes), `.gitattributes` (`/libs/tests/expected/** -text diff`),
  `tools/testinfo/rules.txt` (a rule for `libs/tests/expected/**` selecting
  both tests), the documents the grep `tape\.h|tape_|LIBSTAPE|tapes/`
  finds (starting list: `TODO.md`, `docs/libs/`, `docs/rwelibs/README.md`,
  `docs/v2.10/libs.md`, `libs/userlibs/rwe/CMakeLists.txt`), and the three
  spec deltas of this change. `libs/acidlibs/brexxlib.c` names its own
  BREXX tapes, not the test tape, and is not touched.
- No library or compiler output changes: the library files stay
  byte-identical (`prev_results`).
