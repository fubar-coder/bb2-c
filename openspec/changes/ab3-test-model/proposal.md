## Why

The run-output cases of `tests/ab3` (`ab3_cli` on blitz2-ab3, `ab3_cli_ref`
on AmiBlitz3 v3.14.0) are the last large group of program-output tests
outside the BASIC test model: each case is a program whose `NPrint` lines
are compared with `NAME.expect` / `NAME.ref.expect`. The spike
(`bb2-c-work/ab3-testmodel-spike/report.md`) showed that the test library
and the driver of `corpusrun case --write-driver` run unchanged under both
compilers and that bundled cases are faster. The user chose route B for
these cases, now, before `ab3-mode-g5`, as an OpenSpec change (rounds/tree
decisions of 2026-11-27, "tests/ab3 route; wclash difference" and "tests/ab3
route B: now, before ab3-mode-g5, as an OpenSpec change"), and that testpp
learns AB3's `@@` line joining as part of route B ("tests/ab3 spike result:
the two AB3 differences, testpp `@@`").

## What Changes

- The runner (`corpusrun case`, `tools/lib/testconf`, `tools/lib/testreg`)
  gets two compiler values: `ab3` (blitz2-ab3 from the command line,
  `blitz2-ab3 -s DRIVER -e EXE [-r]`) and `amiblitz3` (AmiBlitz3 v3.14.0's
  own CLI compiler), the staging of the `Blitz3:` tree (the default tree
  only) in the session, the driver as an ASCII source (`.ab3`, or `.bb` with
  the new run setting `source-mode = "bb2"`), and the `compile` query for a
  command-line compile.
- A third side `oracle` (`PREFIX_ID__CASE_oracle`, label `oracle`) for the
  cells of `compiler = "amiblitz3"`, outside `emulator` and `all` as the
  `ab3_cli_ref_N` blocks are today.
- A difference of AmiBlitz3 is a declared deviation of the `amiblitz3`
  cells that names its row of `docs/ab3/variant.md` (key `variant`) instead
  of an issue; it never makes a case `bug-orig`.
- The cells of `compiler = "ab3"` may share a session with other `ab3`
  sources (bundles), so the moved cases are not slower than their blocks.
- The prev comparison of the model covers the tests of `tests/ab3/model/`
  (their drivers compiled by both commits' blitz2-ab3).
- testpp reads AB3's `@@` line joining in a source of AB3 mode (a check
  macro continued with `@@`); `@@` outside the check macros reaches the
  compiler as written. BB2-mode sources are preprocessed as today.
- testpp writes the leading AB3 header block of a test source (`; KEY =
  VALUE` lines AmiBlitz3 reads, `RuntimeDebug`, `Residents`, `Version`) at
  the start of the AB3 driver.
- About 97 run-output cases without arguments and the 4 hand-written `-r`
  cases of `tests/ab3/cli` (the list in design.md D10)
  become `Case` blocks of test files in `tests/ab3/model/`, registered as
  `ab3_ID__CASE` (port side) and `ab3_ID__CASE_oracle`; their
  `ab3_cli_case(...)` lines, `.ab3`/`.bb` sources, `.expect` and
  `.ref.expect` files are removed in the same commits.
- The 54 cases of the expression matrix in `tests/ab3/cli/exprmatrix/` are
  written by `exprmatrix` as model tests of `tests/ab3/model/exprmatrix/`
  (decided: Q2 of rounds/tree decisions, "2026-11-27 ab3-test-model Q1-Q5", "Ja, jetzt (Recommended)"; group 7 of the tasks).
- **Not in scope** (they stay `ab3_cli`/`ab3_cli_ref` cases, recorded
  decision "tests/ab3 route; wclash difference", and the further groups of
  design.md D10 by Q1 of rounds/tree decisions, "2026-11-27 ab3-test-model Q1-Q5", "~97 + 4 -r-Fälle
  (Recommended)"): the compile-error, warning and
  console cases, the cases with another tree, other arguments than `-r`, a
  fixed date, `NORUN`/`REF_NORUN`, `SAME` and `HUNKS`; AmiBlitz3's own test
  programs fetched from `archive/amiblitz3/` (CLAUDE.md rule 7 and
  "License"); the cases whose subject is the compiler's input files
  (includes, residents by path, a tokenised main source with its `.xtra`)
  and `noend`. Also not in scope: `!RunProgram` cases sharing a run (its own
  decision), the bug-port form of AB3-mode defects beyond a sentence in the
  README, `ab3_tokens`, `ab3_tokenise`, `ab3_bb2_equivalence*`.
- No compiler, library or TED source changes; no result of rule 3 changes
  (design.md D12 says how that is shown).

## Capabilities

### New Capabilities
- `testing/ab3-tests`: the tests of AB3 mode: which cases are tests of the
  model and which keep the command-line form, the AmiBlitz3 side as the
  oracle, the differences of AmiBlitz3 as declared deviations with their
  `variant.md` row, and the proof that a moved case checks the values of
  its expect file.

### Modified Capabilities
- `testing/basic-test-runner`: compilers `ab3` and `amiblitz3`, the run
  setting `source-mode`, the side `oracle`, the `compile` query of a
  command-line compile, `@@` and the header block in an AB3 driver, the
  bundles of `ab3` cells (requirement "The cases that may share a session
  with other sources"), and the registration of a third side
  ("Registration at build time without the emulator").
- `testing/previous-commit`: "The tests of the BASIC test model are
  compared with the prev build" also for `tests/ab3/model/`.
- `testing/expression-matrix`: "AmiBlitz3 checks the agreement cells" in
  the model form (Q2 of rounds/tree decisions, "2026-11-27 ab3-test-model Q1-Q5").

## Impact

- Code: `tools/lib/testconf`, `tools/lib/testreg`, `tools/lib/testpp`,
  `tools/lib/testjudge` (side, deviations), `tests/corpus/runner/caserun.c`
  and its session script, `cmake/basic-test-model.cmake`,
  `tools/lib/exprmatrix` (group 7), their host tests.
- Tests: `tests/ab3/CMakeLists.txt` (the moved `ab3_cli_case` lines go),
  `tests/ab3/cli/` (sources and expect files of the moved cases go),
  new `tests/ab3/model/` with `tests.toml`, `ID.test.bas`, `ID.test.toml`.
- Docs: `tests/README.md` ("The test model", "The run configuration",
  "Original material and presets", "The prev build", "bug-port tests"),
  `docs/testing.md`, `docs/ab3/variant.md` (the case names in the
  "Differences" rows point to the model tests), the Purpose of
  `openspec/specs/testing/test-model/spec.md` ("Not yet moved").
- Open changes: `ab3-mode-g5` adds its run-output cases in the new form;
  its tasks naming `ab3_cli`/`ab3_cli_ref` are adjusted before it starts
  (task 9.1). Shared files with `ab3-mode-g5`, `console-break-option` and
  `blitz-coverage`: design.md D15.
- Emulator time: `all` gets the port side of the moved tests instead of
  their share of `ab3_cli_N`; `oracle` the AmiBlitz3 side. Measured in
  task 8.3 against the blocks.
