# Tasks

No original bug: no bug-orig test, no `Fixes #N`. Builds on
`basic-test-bundle-key` (merged first).

## 1. Preparation

- [ ] 1.1 Read whether `amiga_emulation` gives its snapshot key without a session (K3 of `basic-test-bundle-key`). Verify: the report gives the answer.

## 2. Tests first

- [ ] 2.1 `test_verdicts.c` (host): put/claim round trip; a claim of a missing entry; two claimants (one wins); a foreign key; a malformed entry (cut after `key=`, a wrong `v=`, no `status=`): named and not served (the broken-input test of the check); `vstore_clear` keeps an entry another live process holds. Verify: fails on main.
- [ ] 2.2 The stale-key test (host): the key of a member changes when one byte of the source, of a `tests.toml`, of a file below `ID.test/`, of the environment file, of a `[runner]` file or of the runner binary stand-in changes, and not when an unrelated file changes. Verify: fails on main.
- [ ] 2.3 `test_caserun.c`: the chunking of 1, 16, 17 and 33 cells; the bundle session's volume list (one `BUNDLE:` volume) and script (`Assign CELLn: BUNDLE:cN` per cell, then the parts as today); the arm path `c0/out/...` (decision 3b); a mount named `BUNDLE` refused (3c, the broken-input test); the budget of decision 4; the bundle's exit status (I9) from member verdicts. Verify: fails on main.
- [ ] 2.4 An emulator self-test of the bundle (`tests/corpus/runner/CMakeLists.txt`, label `emulator`, a few sources below the runner's test data): a passing member, a failing one, an open bug's REPRODUCED member (with `--require-deviation`), a member that hangs in the middle (its session ends after hang-after, not the time limit: the hang check armed, decision 3b); then the case tests with `--from-bundle`: served for the clean ones, alone for the failing and the hanging one, and a source touched between bundle and case test runs alone. Verdicts equal to the runs alone. Verify: fails on main.

## 3. Implementation

- [ ] 3.1 `run_set` split into phases (design decision 1); stand-alone runs unchanged. Verify: `test_caserun`; `ctest -L emulator -R '^(blitz2_|regression_)'` unchanged.
- [ ] 3.2 Bundle mode: options (`cmd_case.cpp`), members, pooling, the bundle volume, staging by rename, the arm path and the mount check (decisions 3-3c), budget, work directory, per-member skip, report (decisions 2-6, 9). Verify: 2.3 passes.
- [ ] 3.3 Store and key (`verdicts.[ch]`, decision 7; the inputs of I6 and K3; the placeholders of the planning choice). Verify: 2.1, 2.2 pass.
- [ ] 3.4 Served mode (decision 8, I7) and the `@stat` naming (I8). Verify: 2.4 passes.

## 4. Checks

- [ ] 4.1 `ctest --preset host` (and with `-DHOST_SANITIZE=ON`); `ctest --preset all -R '^(blitz2_|regression_|corpusrun_)'`. Verify: pass; no result changes (rule 3).
- [ ] 4.2 Measurement: one bundle of 16 members of the first set against their 16 runs alone (session time, wall time), under the load of the run. Verify: the numbers in the report.

## 5. Documents

- [ ] 5.1 `tests/corpus/runner/README.md` (bundle and served mode, the store), `caserun.h` head comment, `tests/README.md` "Running and reading a test" (a served case's output) and "Preconditions of the program's environment" (the sentence on `CELL0:`: in a bundle a path in `CORPUS:` is `BUNDLE:cN/...`, and a cell can see the other cells' directories). Verify: coordinator's review.

## 6. Commit and report

- [ ] 6.1 One commit. Report: the spike's results, the measurement, the files, the inputs of the key as built. Verify: `git show --stat`.
