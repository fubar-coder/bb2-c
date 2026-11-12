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

## What Changes

- Every test program loads exactly one library set (`NAME.obj` of the C
  build or `NAME.orig` of the original) and writes its observations as
  output: one line per check (check name, input or vector, value). The
  scenario code no longer has the `[2]` arrays, the `cur` loop and the
  second `sets[]` entry.
- A capture run with the original produces the reference, committed as one
  readable text file per test program (`libs/tests/captures/NAME.txt`). The
  run with the C build must give the same output, compared line by line; a
  difference names the check and both values.
- Block hashes replace values only where the volume demands it
  (`ridisasmlib`, 3.06 million observations; `systest_med`, 3.57 million,
  counts from the `end` lines of the tapes): a block of observations is one
  line with its hash, so a difference names the block.
- Pointers stay normalised as the tape does it (relative to the test
  program or a base), written as such in the line.
- The test program checks the registers the Blitz convention requires to
  be kept and writes kept/lost per call; the output is identical between
  original and C, no subset rule in the comparison.
- The two non-deterministic parts become deterministic (`riencryptlib`'s
  random wheel: fixed seed; `ciatrklib`'s DMACONR in the F00 phase: defined
  hardware state); what cannot be reproduced is written as an explicit
  `not captured: REASON` line.
- `expect_fixed` becomes an exception row in a table like
  `tests/corpus/notes.tsv` (check, issue `#N`, the original's value, the
  port's value); a stale row fails.
- Placement: the compare run (C build against the committed captures) runs
  in `all` and `emulator`, no CD. The capture run with the original and the
  check that a fresh capture equals the committed one run in `originals`
  (needs the CD).
- Removed: the tape (`tape.c`, `tape.h`, `tapes/`, `tape-compare.cmake`,
  the `LIBSTAPE` modes, the `tape_*` calls, `tape_pause`, `tape_orig`) and
  the replay mode that loads the C build as `NAME.orig`.
- **BREAKING** for test authors: the macros of the test programs change
  (`SAME`, `SAMEBYTES`, `DO2`, `cmp_preserve`, `check_fixed`,
  `SAME_FIXED`), and `tests/README.md` "Libraries" is rewritten.

Out of scope, a later separate change: explicit setup/act/verify asserts
for the pure compute libraries (a test that states the expected result
itself instead of capturing the original's).

## Capabilities

### New Capabilities
- `testing/library-capture`: the single-library test run, the capture
  format, the kept-register lines, determinism, and where the compare and
  capture runs belong.

### Modified Capabilities
- `testing/expected-results`: the requirement "Per-check fixed values in
  library pair tests" says where the original's side comes from (the
  committed capture), how an exemption is stated (a row naming the issue,
  the original's value and the port's value) and when a row is stale.

## Impact

- `libs/tests/`: `libtest.c`, `systest*.c`, `rwe/*.c` (57 files),
  `rwe/rwecmp.h`, `sysload.h`, `selftest_fixed.c`, `libs_tests.cpp`,
  `emulator.cmake`, `CMakeLists.txt`; new `capture.c`/`capture.h`,
  `capture-compare.cmake`, `captures/`, the exceptions table; removed
  `tape.c`, `tape.h`, `tape-compare.cmake`, `tapes/`.
- `libs/tests/bugorig.c` and the `bug_orig_*` library tests use `sets[]`
  with `NO_ORIGINALS`; they keep their form unless the shared header change
  touches them.
- ctest: `libs_emulator` (compare), `libs_emulator_originals` (capture and
  equality with the committed files), new tests of the checks with broken
  input; `libs_emulator_tape_wrong` and `libs_tapes_compare_*` go away.
- `tests/README.md` ("Libraries"), `CLAUDE.md` (names the tapes),
  `openspec/specs/testing/expected-results/spec.md` via the delta, comments
  that name the tape (`libs/acidlibs/brexxlib.c`, the test programs).
- No library or compiler output changes: the library files stay
  byte-identical (`prev_results`).
