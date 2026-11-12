# Proposal

## Why

The shared test model (`openspec/specs/testing/test-model/spec.md`) was
implemented for the library tests only; `docs/testing.md` ("How they
differ from the model", "Planned follow-ups") lists bug-orig as the first
follow-up. Today the bug-orig tests differ from the model in four ways
(all measured on main):

- `bug_orig_record_ID` is a ctest test (label `originals`, environment
  `BUGORIG_RECORD=1`; `tests/bug-orig/CMakeLists.txt:315-319` for Blitz
  programs, `:558-564` for TED) that writes `ID.orig` into the source tree on
  every `ctest --preset originals`. The runs of 2026-11-11/12 rewrote
  `blitzcoplib-maxslice-zero.orig`, `bb_blitlib-createshape-count0.orig` and
  `rigfxlib-cpucls-no-plane-crash.orig` each time; they were restored by
  hand. The fourth varying file is `rishapeslib-read-palette-colour-count`
  (#820).
- A recording is not checked for determinism. The runner offers wildcards
  (`*` at the end of a line, `...`), but 0 of the 247 `.orig` files use one.
- After a fix, the commit deletes `ID.orig`
  (`tests/bug-orig/check-fixed-orig.cmake` refuses a stale one), so the
  original's side of a fixed bug is not kept and never checked again; the
  model keeps it as the `[orig]` case.
- `fix-scope` (`tools/results fix-scope`) reads only
  `libs/tests/expected` (`cmake/fix-scope.cmake`, `DIRS`); a fix that edits
  `ID.expect` or adds `; @fixed #N` is not checked against its issue.

## What Changes

- `bug_orig_record_ID` becomes compare-only: it observes the original and
  matches it against the committed `ID.orig` (the matcher that already
  handles wildcards), and never writes a file.
- A recording build target `bug-orig-record` (filterable by test id) writes
  `ID.orig` from the original, but only if two runs with a different memory
  layout (`inject.c` option `pad`) agree; otherwise it reports "test not
  deterministic" with the differing sections and writes nothing.
- A line of `ID.orig` that legitimately varies is a wildcard line with a
  stated reason (a comment line before it); the four known files get theirs.
- A fixed test keeps its `ID.orig`: `ID.orig` is the `[orig]` case,
  `ID.expect` the `[fixed #N]` case (the pair, design D1), and the
  originals test of a fixed test compares the original against `ID.orig`.
  `check-fixed-orig.cmake` is replaced by the opposite check.
- `fix-scope` covers bug-orig: a test is projected onto cases and the
  existing libscmp fix-scope judges them.
- Documents: `docs/testing.md`, `tests/README.md`, the site pages
  `site/content/docs/project-structure/tests.md` and
  `site/content/docs/compared-with-original/comparing.md`, `TODO.md`,
  `tools/testinfo/rules.txt`.

## Capabilities

### Modified Capabilities
- `testing/bug-orig`: compare-only record test, recording target with
  agreement check, wildcards with a reason, `ID.orig` kept after a fix,
  mapping onto the shared terms.
- `testing/bug-fix-workflow`: the fix-scope check covers bug-orig tests.
- `testing/original-comparisons`: the compare test of a bug-orig test is a
  determinism and CD-change detector.

## Impact

- `tests/bug-orig/CMakeLists.txt`, `check-fixed-orig.cmake` (replaced),
  runner code in `tests/corpus/runner` (`bugorig.c`, `cmd_pass.cpp`,
  `corpus.h` pass keys) and `tests/ted-ab/runner/run.c` (record mode of the
  TED tests), a new C module under `tools/lib/` for the case projection and
  the acceptance of two runs, `tools/results` (`bug-orig-accept`,
  `fix-scope` extension), `cmake/fix-scope.cmake`.
- Dependency: another agent is changing `tools/lib/libscmp` right now (change
  `libs-capture-host-blocks`, host-side block hashing). This change plans
  against the fix-scope interface as it is on main
  (`libscmp_fix_scope`, `libscmp_fix_scope_dirs`, `struct libscmp_file`,
  `tools/lib/libscmp/libscmp.h:214-258`); task 1.1 re-reads the interface
  before starting and adapts if it moved.
- Order: the change `corpus-test-model` is written against the text of
  `testing/original-comparisons` that this change produces; apply and
  archive this change first.
- Results (executables, TED files, libraries): unchanged; acceptance stays
  byte-identical to the `prev` build.
- No program behaviour changes; only tests and test tools.
