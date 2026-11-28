# Tasks

One topic per commit (CLAUDE.md, "Git"): each numbered task that changes
files is one commit unless it says otherwise. No commit of this change
changes a result of rule 3 (no compiler, TED, library or executable
changes): every commit is checked with the prev build and without
`PREV_INTENDED`. Infrastructure only in C and CMake (rule 6): the parsers
and the judging in C under `tests/corpus/runner` and `tools/lib`, CMake
only registers. Agents run the tests their task names, the tests of what
they touched and `ctest --preset host` (D-026). The recorded answer:
tasks/decisions.md, "2026-11-28 Compiler warnings in the test model".

## 1. The queries in the configuration

- [x] 1.1 `tools/lib/testconf/testassert.c`, `testconf.h`: the queries `compile.warnings` (predicates `==`/`!=` string, `contains`, `matches`, `exists`, `count CMP integer`), `compile.warnings.K` (K decimal from 1, no leading zero; the predicates without `count`), `compile.rc` (`CMP integer`, `exists`). Verify: `test_testassert.cpp` — each query parsed with each allowed predicate; refused, naming file, line and query: `compile.warnings.0`, `compile.warnings.02`, `compile.warnings.x`, `count` on `compile.warnings.1`, `contains` on `compile.rc`.
- [x] 1.2 `testmatrix.c`: `compile.rc` refused in a cell that does not compile from the command line, `compile.warnings`/`compile.warnings.K` refused in a cell of `amiblitz3` (assertion, deviation and alternative end; naming file, line and the compiler). `testconf.c` `check_jobline`: the IDs `warnings` and `rc` refused. Verify: `test_testconf.cpp` with a broken configuration for each refusal; an accepted one with `exclude = ["compiler=amiblitz3"]` and one with the selector `compiler=ab3`.

## 2. The observations

- [x] 2.1 `tests/corpus/runner/caserun.c`/`.h`: the parser of the editor path (the WARNING lines of `j000`'s block: `Warning: line N: TEXT [NAME]`, `Warning: NAME: K more`, `Warning: TEXT`), the line mapped as `cr_compile_mapped` maps it (`N`, `driver:N`); the names from `shared/ab3job/warnlist.h`. Verify: `test_caserun.c` — one warning, the nine lines of the limit with the `more` entry, a resclash-form line, a driver line outside the source, no WARNING line (empty list), no block (not captured); broken input: a WARNING line without `Warning: `, `line` without a number, an unknown `[NAME]`, each cannot-be-judged naming the line.
- [x] 2.2 The parser of the command line (`SOURCE: warning: MESSAGE [NAME]`, `SOURCE:LINE: warning: ...`; the driver's path mapped, another file kept as printed; ANSI escapes and `\r` removed as `plain_line` does), and `compile.rc` from the last `rc=` line; no `rc=` line: both not captured. Verify: `test_caserun.c` — the `.expect` lines of `datafrac`, `datafrac_many` and `datafrac_inc` with the driver's path, a warning without line, an error line beside a warning (only the warning listed, `compile` the error), `rc=5`; broken input: `warning:` without `[NAME]`, an unknown name, `rc=` without a number.

## 3. The judge

- [x] 3.1 `tools/lib/testjudge`: `tj_obs` gets the warnings list and the compile's return code; the queries judged (`count` the number of entries, `compile.warnings.K` beyond the list not captured), both added to `whole_program()`; the report names the query, the expected and the actual list. Verify: `test_testjudge.cpp` — a missing, an added, a reordered warning each fail; `count == 0` on the empty list holds; `compile.warnings.3` of a list of two fails as not captured; `compile.rc == 5` holds and `== 0` fails; a case asserting the warnings runs on its own (`tj_case_alone`).
- [x] 3.2 `caserun.c` fills the new fields for both paths. Verify: `ctest --preset host`, also with `-DHOST_SANITIZE=ON`.

## 4. Documentation of the queries

- [x] 4.1 `tests/README.md`, "Assertions": the grammar (`"compile.warnings" [ "." K ] | "compile.rc"`), three rows of the query table, a paragraph on the entry form (PLACE, NAME, TEXT, `- NAME K more`, the order, how a missing warning and the order are asserted, no default, the refusals), a `toml` example (it is checked by `testconf_readme`); the jobline paragraph of "The run configuration" names `warnings` and `rc`. Verify: `testconf_readme` and `ctest --preset host`.

## 5. The tests in the model

- [x] 5.1 Move `datafrac`, `datafrac_bb2`, `datafrac_many` to `tests/ab3/model/warn-datafrac.test.*`, `warn-datafrac-bb2.test.*` (`source-mode = "bb2"`), `warn-datafrac-many.test.*` (design D7): each `ID.test.toml` with `exclude = ["compiler=amiblitz3"]`, every warning line of the `[compile]` section as `compile.warnings.K` with the new source line, `count == N`, `compile.rc == 5`, every `[run]` value as a check; remove the cases from `tests/ab3/CMakeLists.txt` and their `.ab3`/`.bb`/`.expect` files in the same commit, after both forms passed on the same build. `datafrac_inc` the same way if its include stages (D7), else it stays and 6.1 names the reason. Verify: `ctest -R 'ab3_warn-datafrac'` and the old `ab3_cli` cases on the build before the removal; the prev comparison of the new tests without `PREV_INTENDED`.
- [x] 5.2 `tests/blitz2/warn-datafrac-limit.test.toml`: the nine entries as `compile.warnings.1` .. `.9` with the source lines 14 to 21 and `- datafrac 2 more`, `count == 9`; the deviation of `compiler=orig195` `"compile.warnings" = "count == 0"`; `extension = 1235` kept. Verify: `ctest -R 'blitz2_warn-datafrac-limit'` on both sides (`all` and `originals`).
- [x] 5.3 The deliberately broken runs (CLAUDE.md, "Every check has a test with deliberately broken input"): a copy of `warn-datafrac` and of `warn-datafrac-limit` with one expected entry on a wrong line must fail naming the query (run once, not committed, or as a WANT_FAIL test where the suite has the form). Verify: both fail with the query and both lists in the report; the result named in the commit message.
- [x] 5.4 The comment above the `ab3_cli_case` list in `tests/ab3/CMakeLists.txt` and the remaining datafrac comment: the warning cases are in the model; `datafrac_r`, `datafrac_off`, `datafrac_werr` stay for `-w`/`-werr` and `SAME`. Verify: review of the diff; the remaining `ab3_cli` datafrac cases pass.

## 6. The "Not yet moved" notes

- [x] 6.1 `tests/README.md`, "The test model" and "Tests of AB3 mode": the `ab3_cli` reasons updated, not dropped: "compile errors and warnings judged on the compiler's console output" becomes the compile-error cases and the warning cases with other arguments (`-w`, `-werr`) or `SAME`; the warning cases in the model named. Verify: review of the diff.
- [x] 6.2 The Purpose of `openspec/specs/testing/test-model/spec.md`: the same in its "Not yet moved" item (with the archive's sync; a task note for the archive if the sync does it). Verify: `openspec validate test-model-warnings --strict`.

## 7. The other changes

- [x] 7.1 A note in `openspec/changes/type-system-rework/tasks.md`, task 3.5 (the coordinator, through `openspec-update-change`): its warning tests are written with the queries `compile.warnings`, `compile.warnings.K` and `compile.rc` of this change — the editor path in `tests/blitz2` (cells `port` and `orig195`, `extension = 1235`), the console in `tests/ab3/model` (cell `ab3`) —, and its check "the executables unchanged with the warnings disabled" stays an `ab3_cli` case with `SAME` (as `datafrac_off`). Verify: `openspec validate type-system-rework --strict`.
- [x] 7.2 ab3-mode-g5 task 4.3 as the user answered Q1 (design.md, Answered Questions: the procend case in the model, this change before ab3-mode-g5 group 4) (the coordinator, through `openspec-update-change`). Verify: `openspec validate ab3-mode-g5 --strict`.

## 8. Checks

- [x] 8.1 The agent's checks: the host tests of groups 1-3, `ctest --preset host` with and without `-DHOST_SANITIZE=ON`, a clean rebuild without warnings, the tests of group 5, the `ab3_cli` cases left, the prev comparison of the new and changed tests without `PREV_INTENDED`. Verify: the logs' summaries in the report.

## Workflow follow-up

- The coordinator's acceptance once per batch (`all`, `bug-orig`, `originals` with a clean rebuild; D-010, D-026).
- Archive: sync the deltas of `testing/test-model`, `testing/basic-test-runner`, `testing/ab3-tests`; task 6.2 if the sync does not carry the Purpose.
