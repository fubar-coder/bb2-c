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

Capture and test are separate operations, each possible with either
library set:

| Operation | Library set | Name | When | Result |
|---|---|---|---|---|
| Capture | original | build target `libs_capture` (run on purpose, not a ctest test; needs the CD) | a scenario or the original's files change | the committed reference `libs/tests/captures/NAME.txt`, written only if two runs with different memory layouts agree |
| Capture | C build | the same target with `LIBSET=obj` | debugging only | files in the build directory, diffed against the reference, never committed |
| Test | C build | ctest `libs_test` (label `emulator`; preset `all`; no CD) | every run of `all` | the C run equals the committed captures (exceptions applied) |
| Test | original | ctest `libs_test_originals` (label `originals`; needs the CD) | `ctest --preset originals` | the original's run equals the committed captures (no exceptions); a failure means the test is not deterministic or a CD file changed |

- Every test program loads exactly one library set (`NAME.obj` of the C
  build or `NAME.orig` of the original), chosen by the variable `LIBSET`,
  and writes its observations as output: one line per check (check name,
  input or vector, value). The scenario code no longer has the `[2]`
  arrays, the `cur` loop and the second `sets[]` entry.
- The reference is one readable text file per test program. `libs_test`
  and `libs_test_originals` use the same comparison code; a difference
  names the check and both values. The exceptions table applies only in
  `libs_test`; in `libs_test_originals` the original's value is expected.
- `libs_capture` with the original runs every program twice with different
  memory layouts (a padded allocation before the sets load, so heap and
  slot addresses shift). If the outputs differ it reports the differing
  lines as "test not deterministic" and writes nothing.
- Block hashes replace values only for `systest_med` (3.57 million
  observations) and `rwe_ridisasmlib` (3.06 million; counts from the `end`
  lines of the tapes): a block of observations is one line with its hash,
  so a difference names the block. `libtest` and every other program write
  plain lines.
- Byte buffers up to 64 bytes are written as hex in the line; larger ones
  as length and FNV hash, with a dump on demand (`LIBS_CAP_DUMP`) to find
  the differing byte.
- Pointers stay normalised as the tape does it (relative to the test
  program or a base), written as such in the line.
- Kept registers: per routine the test program states the set the original
  keeps in all calls (the intersection); the program checks that set and
  writes `kept` or `lost REGS`; the output is identical between original
  and C, no subset rule. A register the original keeps only in some calls
  is not covered; this is accepted.
- The two non-deterministic parts (`riencryptlib`'s random wheel,
  `ciatrklib`'s DMACONR in the F00 phase) are made deterministic where
  possible, else written as `not captured: REASON` (decided during
  implementation).
- Task 1 runs every program several times against the original, before
  any conversion, to list today's non-deterministic checks.
- `expect_fixed` becomes an exception row in a table like
  `tests/corpus/notes.tsv` (check, issue `#N`, the original's value, the
  port's value); a stale row fails.
- Renames: `libs_emulator` becomes `libs_test`; `libs_emulator_originals`
  becomes `libs_test_originals` (it no longer re-captures, it only
  compares); the re-capture of the originals becomes the target
  `libs_capture`; `libs_emulator_tape_wrong` becomes `libs_test_wrong`;
  `libs_tapes_compare_*` go away; `libs_expect_fixed*` become
  `libs_compare_row_*`.
- Removed: the tape (`tape.c`, `tape.h`, `tapes/`, `tape-compare.cmake`,
  the `LIBSTAPE` modes, the `tape_*` calls, `tape_pause`, `tape_orig`) and
  the replay mode that loads the C build as `NAME.orig`.
- **BREAKING** for test authors: the macros of the test programs change
  (`SAME`, `SAMEBYTES`, `DO2`, `cmp_preserve`, `check_fixed`,
  `SAME_FIXED`), and `tests/README.md` "Libraries" is rewritten.

Out of scope, a later separate change (setup/act/verify): explicit asserts
for the pure compute libraries (a test that states the expected result
itself instead of capturing the original's), libraries that have no
original (port-only), and checks beyond what the original does.

## Capabilities

### New Capabilities
- `testing/library-capture`: the single-library test run, the capture and
  test operations, the capture format, the kept-register lines,
  determinism, and where each belongs.

### Modified Capabilities
- `testing/expected-results`: the requirement "Per-check fixed values in
  library pair tests" says where the original's side comes from (the
  committed capture), how an exemption is stated (a row naming the issue,
  the original's value and the port's value) and when a row is stale.

## Impact

- `libs/tests/`: `libtest.c`, `systest*.c`, `rwe/*.c` (57 files),
  `rwe/rwecmp.h`, `sysload.h`, `selftest_fixed.c`, `libs_tests.cpp`,
  `emulator.cmake`, `CMakeLists.txt`; new `capture.c`/`capture.h`,
  `capture-compare.cmake`, `capture-accept.cmake`, `captures/`, the
  exceptions table; removed `tape.c`, `tape.h`, `tape-compare.cmake`,
  `tapes/`.
- `libs/tests/bugorig.c` and the `bug_orig_*` library tests use `sets[]`
  with `NO_ORIGINALS`; they keep their form unless the shared header change
  touches them.
- ctest: `libs_emulator` becomes `libs_test`, `libs_emulator_originals`
  becomes `libs_test_originals`; new target `libs_capture`; new tests of
  the checks with broken input; `libs_emulator_tape_wrong` and
  `libs_tapes_compare_*` go away.
- `tests/README.md` ("Libraries", the presets table), `CLAUDE.md` (names
  the tapes and `libs_emulator`),
  `openspec/specs/testing/expected-results/spec.md` via the delta,
  comments that name the tape (`libs/acidlibs/brexxlib.c`, the test
  programs).
- No library or compiler output changes: the library files stay
  byte-identical (`prev_results`).
