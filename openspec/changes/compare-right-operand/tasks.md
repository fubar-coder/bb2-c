# Tasks

Starts after `quick-long-arithmetic` (#1178) and the change of #1179 are merged (recorded order, proposal "Dependency and order"); before `byte-fraction-constant` and `power-negative-base`. The two questions of the proposal are answered ("Decisions of the user", 2026-11-22: "Ablehnen wie unter Word (Recommended)", i.e. refused; "Beide Modi (Recommended)"); task 2.1 builds on them.

## 1. Tests first (red)

- [x] 1.1 The bug-orig test `tests/bug-orig/compiler/compiler-compare-right-operand` (`.bas`, `.expect`; header `BUG(orig) (#1182)` with the rule and the expected behaviour): the 30 cases of `fc4` (`~/.local/share/bb2-c-work/tr-1177-float-context/`), plus `NOT` and unary minus of the comparison (`rl.l=lw AND NOT(l1<>l1)`, `rl.l=lb+-(l1=l1)`), a Quick `OR`/`EOR` with a fraction below 1, `If flag AND x=5` with a default-typed `flag` (whole and fractional), a raw Quick left under a Long destination (`rl.l=q+(l1=l1)`, expected by #1178's rules), a Byte and a Word left operand (unchanged). Every line whose port value on main differs from 1.95 (for instance through #1178) is left out or moved to a regression test, so that the test is REPRODUCED, not DIFFERS. The `BUG(orig) (#1182)` comment at `eval5` (`eval.c:947`). Verify: `.orig` recorded with `cmake --preset amiga -DBUG_ORIG_RECORD_FILTER=compiler-compare-right-operand` and `--target bug-orig-record`; `ctest --preset bug-orig -R compiler_compare_right_operand` gives REPRODUCED.
- [x] 1.2 The refused case (`rf.f=fh AND (l1=l1)`, decided: refused): a bug-orig test `compiler-compare-right-operand-refused` whose `.expect` is `[compile]` "Illegal Operator for Type" (the form of `compiler-expr-1188-refused-*`); REPRODUCED before the fix. Verify: `ctest --preset bug-orig -R compiler_compare_right_operand_refused`.
- [x] 1.3 AmiBlitz3 v3.14.0: the test of 1.1 as `.ab3` through `ab3_cli_ref` (not committed); note per case whether AmiBlitz3 has the bug (for `docs/ab3/variant.md`). Verify: the report lists the result per case.

## 2. Implementation

- [x] 2.1 The flag of design decision 1 (B1): set by `compareop`, cleared by `getelement2`, kept by brackets, `NOT`, unary minus; in `eval5` the boolean in D(regat+1) converted up to `left` (design decision 2) before `convtypef`. The late cut of #1178 (Quick left, Long right) untouched. Verify: 1.1 and 1.2 give FIXED after `; @fixed #1182` is added; `compiler-quick-compare`, `compiler-compare-fraction-const`, `blitz2_quick-arith-long` pass.
- [x] 2.2 `; @fixed #1182` in the headers of 1.1 and 1.2; `BUG(orig) (#1182)` becomes `Fixed (#1182)` with the rule. Verify: `ctest --preset bug-orig -R compiler_compare_right_operand` FIXED.

## 3. Checks

- [x] 3.1 `ctest --preset host`; the targeted tests of 2.1; the matrix passes `ctest --preset all -R corpus_exprmatrix` (no matrix cell changes: two operands only); `corpus`, `corpus_runerr`. Verify: all pass.
- [x] 3.2 The CD corpus (`blitz2-cd.tsv`, BB2 and AB3 mode): no source fails to compile that compiled before (the count for the decision "Ablehnen wie unter Word (Recommended)": a CD source with a Float left operand of a bit operator and a comparison on the right is reported to the coordinator before the merge). Verify: the corpus test passes or the report names the source.
- [x] 3.3 Prev comparison (`cmake --preset amiga -DBLITZ2_PREV=ON`): every differing executable has a Long, Quick or Float left operand with a comparison on the right; the list, counted (programs, places), into `PREV_INTENDED` and the commit message. Verify: `ctest --preset all -R prev` passes with `-DPREV_INTENDED`.
- [x] 3.4 `tools/results fix-scope` against `main`: only the cases of the two new tests change. Verify: the tool's report.
- [x] 3.5 Clean rebuild without warnings; `ctest --preset oracle` if `tests/ab3` changes. Verify: the commands pass.

## 4. Documents

- [ ] 4.1 `docs/blitz2/expression-types.md` (the #1182 sentence, `:136`, becomes the fixed rule with before/after), `docs/blitz2/deviations.md` (kind-F row for #1182; the #1177 row's "Not part of it: #1182" goes), `docs/ab3/variant.md` (a row if 1.3 found the bug in AmiBlitz3). Verify: the coordinator's review; links resolve.

## 5. Commit and report

- [ ] 5.1 One fix commit: code, the tests with `; @fixed #1182`, `Fixed (#1182)`, the documents, the changed results named (`PREV_INTENDED`), `Fixes #1182`. Report: severity 1 proposed (Changes results), topic Compiler; the CD count of changed executables and outputs (for D-023). Verify: `git show --stat`; the coordinator's fix-scope run.

## Workflow follow-up

- Coordinator: label `fixed-in-port`, fixed-bugs entry and overview row, `all`/`bug-orig`/`originals` after the merge (D-010), archive order with `quick-compare-types` and `quick-long-arithmetic` (one capability).
