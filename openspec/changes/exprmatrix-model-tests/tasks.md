# Tasks

After `expr-tree-4b-lookaheads` is merged, before `expr-tree-5-operand-typing`
(the user's slot). Open questions 1 and 2 of the proposal answered before
group 3 (the plan assumes the recommended answers). Emulator runs in a
worktree, never in the main checkout; logs in the scratchpad, reports in
`~/.local/share/bb2-c-work/exprmatrix-model-tests/`.

## 1. Spike and baseline (no commit)

- [x] 1.1 By hand (a scratch copy, not the generator): one source per context (`db`, `dq`, `pl`, `pq`, `if`, `i1` unchecked, `i2` checked, `print`) and the largest group (`i1-I-c-agree-01`, 3,321 lines) in the model form of design decisions 2-4, with their `ID.test.toml`; run each on the port and with `compiler=orig195` and `orig210` (CD libraries); verify: every cell's check gives the value of the old `.expect`/`NAME.out`/`.orig` (a differing cell listed with both values), the compile and run time per source and cell, the session time of a chunk of 16 large sources against the plain key's limit; the case name is no token; report `spike.md` with the chosen `cells-per-source` (design decision 7).
- [x] 1.2 Baseline on main (old form, after `expr-tree-4b`): two full runs of `ctest --preset all` and one of `--preset originals` with JUnit output; verify: `testinfo stats` gives the wall time of each run and the session-time sum of the matrix's old tests (design decision 8), written to `measurement.md` ("before").

## 2. The writer

- [x] 2.1 `tools/lib/exprmatrix/expect.c`: the model writer (one `ID.test.bas` per source, its `ID.test.toml` with `runtime-errors`, `isolate` for the `-u` sources, deviations per selector, `fixed-orig = N`; the `Print` sources with `output`; the refused cells as data sources `ID.test/L-R[-c].bas` grouped per operator / issue set), `em_expect_owns` for the new paths; the AmiBlitz3 writer unchanged; verify: `exprmatrix expect` on the committed description writes the tree, `git diff --stat tests/ab3` is empty.
- [x] 2.2 Host tests in `tools/tests/test_exprmatrix_expect.cpp`: a cell per context in its model line; a fixed cell's two deviation keys with the original's value; a library fix's selector `libs=cd`, a compiler fix's `compiler=orig195`/`orig210`; `fixed-orig` of the last issue of two; an open cell without `fixed-orig`; a refused cell as a data source with its `data.` assertion and line; the `-u` source's `isolate`; `committed-broken.cmake` with an edited `.test.toml`, an edited data source and an extra file; verify: `ctest --preset host -R exprmatrix`, also with `-DHOST_SANITIZE=ON`.

## 3. The bug-orig and regression tests (commits (a) and (b) of design decision 11)

- [x] 3.1 The refused cells: regenerate (`expr-matrix-update`), the old `compiler-expr-*-refused-*` and `regression/compiler-expr-refused-*` files go in the same commit; verify: `exprmatrix_committed`, `ctest -R 'compiler_expr.*refused'` on both sides (`all`, `originals`), a deliberately broken `data.` assertion fails, `ctest --preset host`.
- [x] 3.2 The run tests: regenerate; `compiler-expr-ISSUES-*.{bas,expect,orig}` go; verify: `exprmatrix_committed`; `ctest --preset all -R bug_orig_compiler_expr` (served from bundles in the full run, alone with `-R`) and `--preset originals -R bug_orig_compiler_expr`, each test PASSED/FIXED on both sides; a deliberately wrong deviation value fails naming the cell's check; the prev comparison (`-DBLITZ2_PREV=ON`) of the new drivers passes without `PREV_INTENDED`.
- [x] 3.3 Originals mismatches (design decision 4) and shared-session failures (design decision 6) from 3.1/3.2's runs: fixed in the generator or listed in the report with the cell, both values and the cause; verify: two `originals` runs give the same verdicts.

## 4. The corpus (commit (c))

- [x] 4.1 `tests/bug-orig/exprmatrix/tests.toml` (compiler port/orig210, libs cd/clib) and the registration `corpus_exprmatrix_` in `tests/bug-orig/CMakeLists.txt` (PREV `^exprmatrix/`; the `bug_orig_` registration's EXCLUDE extended); verify: `ctest -N -R corpus_exprmatrix` lists the case tests, the `_originals` tests and the bundles.
- [x] 4.2 Regenerate into the new area; `tests/corpus/tests-exprmatrix/*.bas` and `tests/corpus/expect/corpus_exprmatrix*` go; in `tests/corpus/CMakeLists.txt` the `tests-exprmatrix` entry of the tokenising loop, the pass loop and `NO_EXPECT_TARGETS` (grep: no other user; `EXPECT` stays if another pass uses it); verify: `exprmatrix_committed`, configure without warnings, `ctest --preset all -R corpus_exprmatrix` and `--preset originals -R corpus_exprmatrix` pass, the prev comparison passes.
- [x] 4.3 As 3.3 for the corpus (the unmeasured 2.10-with-C-libraries cell in particular); verify: two `originals` runs give the same verdicts.

## 5. Bundles, isolation, equivalence

- [x] 5.1 Bundle membership: `ctest -N` shows which moved tests are bundle members and which run alone (`Print`, `-u`, data sources), listed in `measurement.md`; verify: the counts match design decisions 3, 5 and 6.
- [ ] 5.2 The `-u` index sources bundled on trial (`isolate` off in a scratch regeneration): two full `all` runs; verify: the same verdicts as isolated, then `isolate` dropped by the generator, else kept with the finding in the report.
- [x] 5.3 (by open question 2, recommended answer) The equivalence passes `ab3_bb2_equivalence` and `_runerr` (`tests/ab3/CMakeLists.txt`) also compile the drivers the build writes (`model-drivers/*.bb2` of every registration with PREV, `basic_test_driver`: the moved tests and the bug-orig and blitz2 model tests moved before) and the matrix's data sources, with the test library in `blitz2:userlibs`; verify: both passes pass, a deliberately differing driver is named; commit (d).

## 6. Measurement and removal check

- [x] 6.1 Grep that nothing names the removed files, passes or options (`tests/`, `cmake/`, `tools/`, `docs/`, `site/`); `tools/testinfo/rules.txt` patterns for the new names and paths; `test_covsel.cpp`'s example names; verify: `testinfo` host tests and `ctest --preset host` pass, a clean rebuild has no warnings.
- [ ] 6.2 The measurement after the move as 1.2 (design decision 8) on the branch; verify: `measurement.md` ("after") with both presets; not slower in each preset, else stop and report to the coordinator (open question 3) before group 7.

## 7. Documents

- [ ] 7.1 `docs/testing-old-form.md`: section 3 removed (the table row and the counts of the intro; section 2 without "the generated tests (159)"; the stray `EOF` line at the end of section 3 goes with it), `TODO.md` "Tests left in the old form" without the generated tests; verify: the counts match `ls`.
- [ ] 7.2 `tools/exprmatrix/README.md` ("The committed tests (expect)": the model files, deviations, `fixed-orig`, data sources), `tests/README.md` ("The expression matrix": the tests, bundles, updating without `bug-orig-record`), `docs/ab3/variant.md` (`@fixed-orig #N` of the matrix's tests becomes `fixed-orig = N`); verify: `readme_exprmatrix` passes, links resolve.
- [ ] 7.3 `expr-tree-5-operand-typing` (proposal "the matrix's expected files", design "fails the matrix pass", task 2.1's regex `exprmatrix|operand-order`) adjusted to the model form with `openspec-update-change`; verify: `openspec validate expr-tree-5-operand-typing --strict`; commit (e) with the docs.
- [ ] 7.4 Acceptance by the agent: `ctest --preset host`, the moved tests in `all` and `originals`, `fix-scope` against main (only the moves, no issue case); verify: the commands pass; the coordinator runs `all`, `bug-orig` and `originals` after the merge (D-010).

## Workflow follow-up

- Archive the change; the main spec `testing/expression-matrix` takes the deltas.
