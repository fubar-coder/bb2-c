# Tasks

## 0. Before the implementation

- [x] 0.1 The user's answers to the three questions recorded (`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, "Runner extension: three answers") and folded into the change (the selector form of `isolate`); verify: `openspec validate basic-test-runner-extension --strict`, the answers quoted in proposal.md and design.md D3, D5, D6

## 1. The driver of all cases and the run of all cases

- [x] 1.1 `--print-driver` without `--case` prints the driver of all cases; `cmd_case.cpp` accepts no `--case` for a run and for `--print-driver`; verify: a host test (`test_caserun.c` or a ctest of the runner) that the printed text is byte-identical to the `.bas` that `--write-driver` writes for `tests/blitz2/model-numbering.test.bas`
- [x] 1.2 `caserun()` split into running a set of cases (driver of the set, cells, sessions, observations) and judging a case; the run of one case unchanged in report and exit; verify: `ctest --preset host`, the ctest tests of `tests/blitz2` with the label `emulator` that run `corpusrun case` (`ctest -R 'blitz2_.*__|blitz2_selftest_model'`) pass unchanged
- [x] 1.3 The judge accepts the blocks of the other cases the driver selected (`tj_test` with the driver's case list), a block of an unselected case stays foreign; verify: host tests in `testjudge` (two cases in one result file, each judged from its block; a failed check in b does not fail a; a failed batch cleanup check fails both; an unselected case's block still fails)
- [x] 1.4 The cases that run on their own (D2: whole-program queries other than the defaults in any cell and mode, `!ExpectError` in the case's namespace) as a function with host tests; verify: host tests with broken input for each query kind and for an `!ExpectError`
- [x] 1.5 Clean run, open case, reruns and the confirmation alone (D3); report and exit status (D4); verify: host tests of the clean condition, the open case, the loop's end and the order of verdicts; an emulator ctest `blitz2_selftest_model_all_cases` on a new `tests/blitz2/selftest/model-all-cases.test.bas` (a passing case, a case with a failed check, a case that ends with `End`, a case behind it, a case asserting `output`) whose `PASS_REGULAR_EXPRESSION` checks each case's line, the "(alone)" marks and the summary line, and a second run of it with the side `port` exit status 4

## 2. A cell in a session of its own

- [ ] 2.1 The run setting `isolate` in `testconf` (D5: a string `on`/`off` or a non-empty list of cell selectors checked as `exclude`'s, the value kind of its own, refused as an axis) and in `cr_session_key` (a key of its own per isolated cell); verify: host tests with broken input (`testconf`: another string, an empty list, a malformed or unknown selector, the key in `[matrix]`, each naming file and line; `test_caserun.c`: `"on"` gives three keys for three cells, `["libs=cd"]` two keys for `port,cd`/`orig195,cd` and one shared for `port,clib`), and an emulator test with two cells and `isolate = "on"` in two sessions (the `@stat` lines)
- [ ] 2.3 The run of all cases with isolated cells (D5): every case alone in those cells, the other cells shared, each case judged over all cells of the side, the confirmation alone in every cell; verify: host tests of the plan of runs (which case runs where), and the emulator test of 1.5 with `isolate = ["libs=cd"]` on the side `originals`
- [ ] 2.2 The ctest time limit with isolated cells: measure the session start against the build's margin (`BASIC_TEST_TIMEOUT`, `--timeout-extra`), add a margin per isolated cell in `testreg` only if needed; verify: the measurement in design.md "Findings", a `testreg` host test if it changes

## 3. Alternative ends

- [ ] 3.1 The table `ends."SELECTOR"` in `testconf` (own file only, `reason` required, alternatives non-empty lists of `QUERY PREDICATE`, allowed queries, refused with `fixed`), counted as a declared deviation; verify: host tests with broken input for each refusal (no reason, empty alternative, in a `tests.toml`, with `fixed`, a refused query) naming file and line
- [ ] 3.2 Judging (D6): in the cells where the deviations apply, the named queries leave the assertions and defaults, one alternative must hold, the report names it or each failed one; verify: host tests in `testjudge` (open bug reproduced with "hangs", fixed with "returns", none holds -> failed, a cell without the faulty part of a fixed test judged plain)
- [ ] 3.3 An emulator check of a varying end: a selftest source whose case returns or hangs by a value the cell sets (two cells), with both alternatives declared; verify: the ctest test passes in both cells and fails when one alternative is removed

## 4. The query `inject`

- [ ] 4.1 `observe_cell` fills `inject` with the `inject:` lines of the output when the cell ran through inject (`pad = "on"`, `runwrap` with `@INJECT@`), else not captured; the output unchanged; verify: host test of the extraction in `test_caserun.c`, and an emulator run of a pilot test with `runwrap = "@INJECT@|guard"` (after the pilot's merge, e.g. `shapeslib-loadshape-width-zero`) with an added assertion on `inject`, passing; without inject the assertion fails as "not captured"

## 5. Documentation and acceptance

- [ ] 5.1 `tests/corpus/runner/README.md`, `cmd_case.cpp`'s help and footer, `tests/README.md` ("Tests that judge themselves", the key table, the assertions), the site page `site/content/docs/project-structure/running-tests.md` (running all cases, the report, the exit status, `--print-driver` without `--case`); verify: the texts name the run of all cases, `isolate`, `ends` and `inject`, and no text says that `--print-driver` needs `--case`
- [ ] 5.2 The section "Moving a bug-orig test to the model" of `tests/README.md` (after the pilot's merge): the three pitfalls (inject not filled, own session per cell, varying end) rewritten to the new keys; verify: no text says the query `inject` is not filled or that a varying end cannot be declared
- [ ] 5.3 Acceptance of the agent: `ctest --preset host` (also with `-DHOST_SANITIZE=ON` for the changed host tests), the tests of 1.2, 1.5, 2.1, 3.3, 4.1, a clean rebuild without warnings; no result of the compilers or libraries changes (the prev comparison of the coordinator's `all` run); verify: the run logs named in the report
