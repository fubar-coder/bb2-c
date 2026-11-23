# Tasks

No original bug: no bug-orig test, no `Fixes #N`. K1, K2, G1 and R1 are
answered (`rounds/tb/decisions.md`, "K2, R1, Q17"; "K1, K2 follow-up, G1").

## 0. Spike (G1)

- [x] 0.1 Spike in a separate build directory (no commit; design decision 5): a session with one writable volume `BUNDLE:`, the directories `c0`..`cN` and `Assign CELLn: BUNDLE:cN` before each cell's part, for 1, 4 and 16 cells of the first set (the prototype's sources): the verdicts equal those of the runs alone; the second session of another size restores the first one's snapshot (`snapshot=restored`); the list of design decision 5 checked (which tests of the first set use `CORPUS:/`, `Info`, free space or the volume name). Verify: the report gives (b) works / does not, the measurements and the tests found; the coordinator updates `basic-test-bundle-runner` and `basic-test-bundle-registration` before their marked tasks.

## 1. Tests first

- [x] 1.1 `tools/tests/test_testmatrix.cpp`: a cell without `timeout` and one with the environment's value give the same key; with 30 another key; an environment without `timeout` gives the unset key; the existing cases with the environment passed. Verify: fails to build on main (signature).
- [x] 1.2 Tests of `tj_bundle_case` (in the tests of `tools/lib/testjudge`): a plain case (1), a case that asserts the output (0), an `!ExpectError` case (0), two port cells (0), `isolate` (0), `serial` (0), `timeout = 30` (1), `timeout = 45` (0), `hang-after` set by the test (0), `uae` set (0), a `[data]` entry (0), a run from the editor (0), the side originals (0). Verify: fails on main.

## 2. Implementation

- [x] 2.1 `tc_session_key` with the environment (design decision 1); the callers in `caserun.c`, `test_caserun.c`, `test_testmatrix.cpp`; the comment. Verify: 1.1 passes; `test_caserun` passes.
- [x] 2.2 `cr_case_alone` moved to `tools/lib/testjudge` as `tj_case_alone` with its tests (decision 2). Verify: `git diff --stat` shows the move; `test_caserun` and the testjudge tests pass.
- [x] 2.3 `tj_bundle_case` (decision 3). Verify: 1.2 passes.
- [x] 2.4 `[runner] run-dir` in the environment file (decision 4); `tc_env_runner(env, "run-dir")` read by nothing yet. Verify: the build writes it; `corpusrun case --print-config` of a test unchanged.

## 3. Checks

- [x] 3.1 `ctest --preset host`, also in a `-DHOST_SANITIZE=ON` build of the host tools. Verify: pass.
- [x] 3.2 `ctest --preset all -R '^(blitz2_|regression_)'` and the prev comparison: no result changes (rule 3). Verify: pass, no `PREV_INTENDED`.

## 4. Documents

- [x] 4.1 `tests/corpus/runner/README.md` (the key with the environment's time limit; `tj_case_alone` in testjudge), the comment of `tc_session_key`; a `TODO.md` line for the follow-up change of K2 (bundling cells with `[data]` and runs from the editor, after the first set; with the note of design decision 5 on the editor run's hang check). Verify: coordinator's review.

## 5. Commit and report

- [x] 5.1 One commit. Report: the files, whether any registered test's session grouping changed (a list of the tests with a cell without `timeout` beside one with 90), the spike's result (G1 (b) or (a)). Verify: `git show --stat`.
