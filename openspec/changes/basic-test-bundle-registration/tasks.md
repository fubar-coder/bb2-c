# Tasks

No original bug: no bug-orig test, no `Fixes #N`. Builds on
`basic-test-bundle-key`; task 4 on `basic-test-bundle-runner` (both merged
first). G1: one volume per bundle (option (b); the spike of `basic-test-bundle-key`,
task 0.1, showed it works): no padding, a bundle of one case is ordinary.

## 1. Tests first

- [x] 1.1 Host tests of testreg (`tools/tests/`): 40 plain cases give bundles of 16, 16, 8 (names, member order, commands, DEPENDS); 17 cases give bundles of 16 and 1; different labels, fixtures or keys give different bundles; a case that asserts the output, a two-cell source, an isolated cell and the side originals are in none; `--no-bundle` gives no bundle and no DEPENDS; a bundle with the label bug-orig is ordered by its COST; a measured COST entry of a bundle is applied. Verify: fail on main.
- [x] 1.2 `tools/testreg/tooltest.cmake`: the include file of a small directory with bundles is read by ctest (`ctest -N` lists the bundle and its members; `ctest -N -R CASE` lists the case alone; `ctest -N -L bug-orig` lists the bundles of that label). Verify: fails on main.

## 2. Implementation

- [x] 2.1 Groups, chunks and the bundle tests (design decisions 1, 2). Verify: 1.1 passes.
- [x] 2.2 The members' options and DEPENDS (decision 3). Verify: 1.1, 1.2 pass.
- [x] 2.3 The switch (decision 4) in `tools/testreg/main.cpp` and `cmake/basic-test-model.cmake`. Verify: a build with `-DBASIC_TEST_BUNDLE=OFF` registers the tests of main (the include files equal but for the comment line).

## 3. Checks

- [x] 3.1 `ctest --preset host` (and with `-DHOST_SANITIZE=ON`). Verify: pass.

## 4. With the runner merged

- [ ] 4.1 `ctest --preset all -R '^(blitz2_|regression_)'` with bundles. Verify: pass; the report gives the number of bundles, members and served cases and the wall time against the same selection with `BASIC_TEST_BUNDLE=OFF`.
- [ ] 4.2 The comparison run (decision 5): `all` and `bug-orig`, with and without bundles, the verdict of every member compared. Verify: no difference; the list of compared members in the report. A difference: stop and report.

## 5. Documents

- [ ] 5.1 `tests/README.md` (bundle tests: what they are, that a case test is served or runs alone, selection by `-R`/`-L`, the switch), `tools/testreg/README.md`, `TODO.md` (the TODO line on the generated tests names the bundles as done for the first set; `corpus_exprmatrix` stays open). Verify: coordinator's review.

## 6. Commit and report

- [ ] 6.1 One commit. Report: the numbers of 4.1, the comparison of 4.2, the files. Verify: `git show --stat`.
