# Tasks

Starts after `compare-right-operand` (#1182) is merged and after task 3.5 of `expression-type-matrix`; before `power-negative-base`. The three questions of the proposal are answered ("Decisions of the user", 2026-11-22: "Ablehnen wie unter Word (Recommended)", i.e. refused; "Beide Modi (Recommended)"; matrix cells T1, "Ja für alle" - zumindest solange bis die Test-Infrastruktur fertig umgebaut ist); task 2.1 builds on them.

## 1. Tests first (red)

- [x] 1.1 The red tests exist: `tests/bug-orig/compiler/compiler-byte-fraction-const`, the matrix's `compiler-expr-1188-db-A-01`, `-db-B-01`, `-print-A-01`, `-print-B-01`, the 29 `compiler-expr-1188-refused-*`, and `compiler-expr-1188-1189-db-B-01`. Add to `compiler-byte-fraction-const`: a Byte parameter of a statement and a function, a Byte field and a Byte array element, a negative literal (`rb=(-1.5*vb)`), a literal with an exponent (`1.5e1`), the whole-valued literals of decision 2 (`rb=vb+2.0`, `rb=vb AND 2.0`, unchanged) and `Data.b 1.5` (unchanged; the last two in a regression test `regression/compiler-byte-whole-literal`, they are not bug cases). Verify: `.orig` re-recorded (`-DBUG_ORIG_RECORD_FILTER=compiler-byte-fraction-const`, `--target bug-orig-record`); `ctest --preset bug-orig -R 'compiler_byte_fraction_const|compiler_expr_1188'` REPRODUCED; the regression test passes on main.

## 2. Implementation

- [ ] 2.1 `consttype2` `.byte`: a literal with a non-zero fraction under a Byte type so far to `numdofloat` (design decisions 1, 2); not for data (decision 3: the `data` flag passed in). Verify: the tests of 1.1 FIXED after `; @fixed #1188`; `regression/compiler-compare-fraction-const`, `compiler-quick-compare`, the new regression test pass.
- [ ] 2.2 The #1186 helpers (design decision 4): removed, or kept with the reason in the report. Verify: `regression/compiler-compare-fraction-const` unchanged and passing; prev comparison of 3.3.
- [ ] 2.3 `BUG(orig) (#1188)` in `consttype2` becomes `Fixed (#1188)` with the rule (and the whole-valued exception); `; @fixed #1188` in `compiler-byte-fraction-const`. Verify: `ctest --preset bug-orig -R compiler_byte_fraction_const` FIXED.
- [ ] 2.4 The matrix: `EM_R_1188` into `EM_RULES_PORTNOW` (`tools/lib/exprmatrix/exprmatrix.h`), `--target expr-matrix-update`, the fixed cells stay in their bug-orig tests with `; @fixed #1188` (decided: T1). Verify: `git diff` shows only #1188's cells (and `compiler-expr-1188-1189-db-B-01` named as DIFFERS until #1189); `ctest --preset host -R exprmatrix`; `ctest --preset bug-orig -R compiler_expr_1188` FIXED (the two-issue test DIFFERS).

## 3. Checks

- [ ] 3.1 `ctest --preset host` (also `-DHOST_SANITIZE=ON` if `tools/` changed); `ctest --preset all -R corpus_exprmatrix`; `corpus`, `corpus_runerr`. Verify: pass.
- [ ] 3.2 The CD corpus (BB2 and AB3 mode): no new compile error; a CD source that is now refused (decision 5, a Byte constant context) is reported before the merge. Verify: the corpus test or the report.
- [ ] 3.3 Prev comparison: the changed executables (fractional literals under Byte), listed and counted into `PREV_INTENDED` and the commit message. Verify: `ctest --preset all -R prev` with `-DPREV_INTENDED`.
- [ ] 3.4 `tools/results fix-scope` against `main`: only #1188's cases (the matrix's #1188 cells, `compiler-byte-fraction-const`) and the named two-issue test. Verify: the tool's report.
- [ ] 3.5 Clean rebuild without warnings. Verify: the build log.

## 4. Documents

- [ ] 4.1 `docs/blitz2/expression-types.md` (Byte literals: the fixed rule, the whole-valued exception against Word), `docs/blitz2/deviations.md` (kind-F row for #1188; the #1177 row's #1186 sentence), `docs/ab3/variant.md` (the #1188 rows of the matrix's task 3.5 go: AmiBlitz3 and the port agree now). Verify: coordinator's review; links resolve.

## 5. Commit and report

- [ ] 5.1 One fix commit: code, tests, `Fixed (#1188)`, the regenerated matrix files, the documents, the changed results (`PREV_INTENDED`), `Fixes #1188`. Report: severity 1 proposed, topic Compiler; the CD count. Verify: `git show --stat`; the coordinator's fix-scope run.

## Workflow follow-up

- Coordinator: label, site entry and row, the acceptance runs after the merge (D-010), archive order (one capability with three other changes).
