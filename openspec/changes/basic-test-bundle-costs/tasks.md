# Tasks

No original bug: no bug-orig test, no `Fixes #N`. Needs only the interface
I7/I8 of `basic-test-bundle-key`.

## 1. Tests first

- [x] 1.1 Host tests (`tools/tests/`, JUnit files written by the test): a case served in every run keeps its entry with `--previous` and gets none without; a case served once and alone twice gets the median of the two; a bundle with warm sessions gets its entry; a malformed `@served` line is reported (exit 1) and the rest counts; `testinfo stats` counts the served runs and leaves them out of the durations. Verify: fail on main.

## 2. Implementation

- [x] 2.1 `@served` in `teststats` (design decision 1). Verify: 1.1 (parse cases) passes.
- [x] 2.2 `--previous` and `--update` (decision 2). Verify: 1.1 passes.
- [x] 2.3 `testinfo stats` (decision 3). Verify: 1.1 passes.

## 3. Checks

- [ ] 3.1 `ctest --preset host` (and with `-DHOST_SANITIZE=ON`); `testinfo costs` of the last `junit-all.xml` equals `tests/data/test-costs.txt` where the file was refreshed from it (no served runs in it). Verify: pass.

## 4. Documents

- [ ] 4.1 `tools/testinfo/README.md`, `tests/README.md` "The measured COST" (served runs, `--previous`, the full-refresh command). Verify: coordinator's review.

## 5. Commit and report

- [x] 5.1 One commit. Report: the files. Verify: `git show --stat`.
