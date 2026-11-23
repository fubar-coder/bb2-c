# Tasks

No original bug is fixed here (no rule-4 workflow); no result changes
(rule 3: no `PREV_INTENDED`). Start from main after the `flaky` work has
merged (design D7). Each task runs the host tests of what it touched and
`ctest --preset host` (also with `-DHOST_SANITIZE=ON` for the host
programs touched); emulator tests as named.

## 1. Configuration (host)

- [ ] 1.1 `tools/lib/testconf`: the IDs of the `jobline` elements (`job=`, `run=`, `menu=`, `resident=`) read per cell; the checks of the spec (form, length 15, unique, not `j000`, `bNNN`, `log`, `inject`, not `r001` with `editor-run = "on"`), each naming file, line and ID; verify: `test_testconf` with broken input for each refusal, and every test of the tree still registers (`ctest --preset host`, the registration tests)
- [ ] 1.2 The queries `compile.log.NAME` and `compile.NAME` in `testassert.c` (parse after the fixed queries; `NAME` a job of the test: j000, r001 of an editor-run cell, a jobline ID, a `basic-compile` file), the refusals of `compile.j000` (naming `compile`), `compile.FILE` (naming `data.FILE`), `compile.ID` of a `run=`/`menu=` step (naming `compile.log.ID`), an unknown `NAME`; predicates as `compile.log` and `compile`; verify: `test_testconf`/testassert host tests, one broken input per refusal
- [ ] 1.3 `@DATA:FILE@`: recognised in `resolve_string` before the environment placeholders, kept in `jobline` for a `basic-compile` data source, refused elsewhere and for another file (file, line, placeholder); `--print-config` shows it unreplaced; verify: `test_testconf` (kept in `jobline`, refused in `files`, refused for an undeclared file)

## 2. Observation and judging (runner, host)

- [ ] 2.1 `tests/corpus/runner/bugorig.c`/`.h`: the block of a step in a tedstub log (design D2: the cut, the size, the version line); verify: `test_bugorig` with logs of a job, a job with `bitmap=` lines, a menu item behind a job (its `ezfreq` line in the menu's block), a resident, a job that did not run, two sessions
- [ ] 2.2 `caserun.c` `cr_observe`: the blocks of the driver's job, the editor run, the jobline jobs (cell log) and the data sources (the part's log where the source has a part; `bNNN` written as `FILE`, the staged path as `@DATA:FILE@`); the results of `job=`/`resident=` jobs; `testjudge`: the two queries ("not captured" without a block); verify: `test_caserun` with recorded cell directories (a cell with j000 and j001, a data source in the cell's job list and one in a part), `testjudge` host tests
- [ ] 2.3 `cr_cell_pass`: `@DATA:FILE@` replaced by the staged path of the data pass (the same count as `cr_data_pass`); verify: `test_caserun` (two data sources, the second named in a job line gets `b001`; a data source in a part gets the part's path)

## 3. Docs

- [ ] 3.1 `tests/README.md`: "The run configuration" (`@DATA:FILE@` beside `@DRIVER@`; the jobline IDs and their rules), "Assertions" (the grammar and table rows of `compile.log.NAME` and `compile.NAME`; how `compile`, `compile.log`, `compile.inject` relate; no per-job inject), "Moving a bug-orig test to the model" (the `[log]` row: the size of `RESULT ID ok` is removed, a block per job with `compile.log.NAME`; `CORPUS:src/bNNN.bb2` becomes `@DATA:FILE@`); `tests/corpus/runner/README.md`; verify: `basictest_readme` and the other README host tests pass

## 4. The users

- [ ] 4.1 `compiler/compiler-panic-remtask` and `rwe/rishapeslib-cludged-shape-pop`: `@DATA:victim.bas@` in `jobline`, the comments without `b000`; verify: panic-remtask REPRODUCED in `ctest --preset bug-orig -R compiler-panic-remtask` and its originals side as declared; rishapeslib keeps its verdicts on both sides (`ctest --preset emulator -R rishapeslib-cludged-shape-pop`, originals with the CD)
- [ ] 4.2 `compiler/compiler-racing-stripe-wide` (#980) to `ID.test.bas`/`ID.test.toml` (design D6), the old files removed and the includes kept as test data in the same commit, `docs/testing-old-form.md` section 5 without it; verify: FIXED on the side port, the original cells pass as declared (with the CD), the prev comparison (`-DBLITZ2_PREV=ON`) passes for it, fix-scope reports only the move

## Workflow follow-up

- The coordinator ticks the racing-stripe-wide part of task 3.3 of
  `basic-test-editor-run` when this change merges, records the Plan
  choices of design.md in `rounds/tr/coordinator-decisions.md`, and runs
  `all`, `bug-orig` and `originals` once after the merge (D-010).
