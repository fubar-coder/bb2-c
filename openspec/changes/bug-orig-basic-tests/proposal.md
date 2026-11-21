# Proposal

## Why

About 1000 bug-orig tests print observations and are judged against
`ID.expect` and the recorded original `ID.orig` (1017 files, most with
the session log of the original's batch run). Of 962 pairs, 542 differ in
one line of the run section, 214 in two; 29 `.orig` need `# varies`
wildcards. The `.orig` files are recorded on the CD with two memory
layouts and must be re-recorded whenever a test changes. With the test
library (`basic-testlib`) and the runner model (`basic-test-runner`) the
expectation is in the program and the original's deviation is declared.
User decisions of 2026-11-21 (chat, `task-decisions.md`): `.expect` and
`.orig` go away, "ein Erfolg ist ein Erfolg".

## What Changes

- **The bug-orig tests whose program runs move to the new model**, in
  rounds by area: `ID.test.bas` with setup / act / verify and named
  cases, a run configuration per directory and per test instead of the
  header directives, the original's deviation declared for the cells with
  the faulty part, with optional validators.
- **BREAKING (for the test sources):** `ID.expect` and `ID.orig` of a moved
  test are removed; `bug-orig-record` and `bug_orig_record_ID` no longer
  apply to moved tests.
- **Runtime errors as the correct result** (121 tests) are announced in
  the program and matched with the requester; exit codes and crashes are
  assertions of the configuration.
- **A fix changes one key** of the test's configuration: `fixed-orig = N`
  for an original bug, `fixed = N` for another bug; the checks and their
  expected values stay as they are.
- **fix-scope** covers the tests of the new model.
- Not moved: the TED bug-orig tests (driver scripts) and the tests whose
  correct result is a compile result only (68 without a run section; a
  later form through TED's proxy or the compiler's command line).

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `testing/bug-orig`: bug-orig tests in the new model, open and fixed;
  the recorded original only for the tests outside it.
- `testing/bug-fix-workflow`: what a fix changes in a test of the new
  model, and the fix-scope check for it.

## Impact

- `tests/bug-orig/**` (about 940 sources renamed to `ID.test.bas` and
  rewritten, their `.expect`/`.orig` removed, configuration files added),
  `tests/bug-orig/CMakeLists.txt`.
- `tools/results` (fix-scope), `tests/corpus/runner` (the verdicts of
  declared deviations).
- CLAUDE.md, `.claude/agents/tester.md`, `.claude/agents/implementer.md`,
  `docs/development.md` ("Fixing an original bug"), `tests/README.md`:
  the wording of the fix workflow (`fixed-orig`/`fixed`); shown to the user
  before it is written.
- No result of the compilers or libraries changes.
- Depends on `basic-testlib` and `basic-test-runner`.
