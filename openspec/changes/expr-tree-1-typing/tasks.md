# Tasks

Starts after #1178 (`quick-long-arithmetic`), #1179, #1182, #1188, #1189 and #1190 are merged (recorded order, proposal). Open question 1 answered before group 3. Not in parallel with any other change that edits `programs/blitz2/src/eval.c` (`compiler-function-optimizer`, the later changes of the series). "The check" is `design.md`, "The check of every byte-identical step".

## 1. Tests first

- [x] 1.1 Read `tools/lib/exprmatrix/model.c` for a callable type-so-far function (design decision 2); write the host test skeleton `programs/blitz2/tests/host/test_exprtype.cpp` with the cross-check cases against the model for the two-operand cells of `tests/corpus/tests-exprmatrix/matrix.txt` (no compiler code yet: the test calls the model only, to fix the cell list); verify: `ctest --preset host -R blitz2_exprtype` builds and its model-only cases pass.
- [x] 1.2 Record the baseline on main: a prev-capable build (`-DBLITZ2_PREV=ON`) and one run of the check's emulator part on the unchanged tree (no difference expected; a difference here is a flaky test to repair first, D-009); verify: the run is clean and its log is kept in the change's report.

## 2. The functions

- [x] 2.1 `programs/blitz2/src/exprtype.c`/`.h`: `exprtype_binop` and `exprtype_element` (design decision 1), with the facts of the merged fixes (#978 byte rule, #1177 comparisons and `cmplq`, #1178 raw Quick and K3, #1179 index, #1182 right-hand comparison, #1188 Byte destination constant, #1189 if it touches typing); compiled into `blitz2` and `blitz2-ab3` and as a host library; verify: clean build without warnings, the file compiles with `-std=c99 -Wall -Wextra -pedantic` on the host.
- [x] 2.2 `eval5`, `doop`, `compareop`, `cmplq`, `varcont`, `consttype2`'s sizing call the functions and emit only what they return; no other edit; verify: the check (no difference anywhere).
- [x] 2.3 The golden table `programs/blitz2/tests/host/exprtype.tsv` written by the test program's generator mode after 2.2's check passed; the host test compares function and table, and function and model (1.1); a broken-input case (a table with one changed row) fails; verify: `ctest --preset host -R blitz2_exprtype` and with `-DHOST_SANITIZE=ON`.
- [x] 2.4 `tools/testinfo/rules.txt`: `exprtype.*`, the table and the test map to `blitz2_exprtype`, the matrix corpus passes and `compiler_prev`; verify: the selection test of `testing/test-selection` with a changed `exprtype.c`.

## 3. Documents

- [x] 3.1 Comments of the new and changed functions in the form of the answer to open question 1; `docs/blitz2/expression-types.md`: a paragraph naming `exprtype.c` as the place of the rules and the golden table as their pin; verify: links resolve, `blitzlib check`/build without warnings.

## 4. Commit and report

- [x] 4.1 One commit (topic: the type decision function, a rework): code, host test, table, rules, docs; message names "no result changes" and the check's runs; the report lists the planning choices of `design.md` and the measured compile time of `corpus_cd` before and after (information, no budget); verify: `git show --stat` contains only the files of this change.
