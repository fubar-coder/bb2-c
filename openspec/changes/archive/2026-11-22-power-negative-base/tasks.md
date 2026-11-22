# Tasks

Starts after `quick-long-arithmetic`, `compare-right-operand` and `byte-fraction-constant` are merged. Questions 1 and 3 of the proposal are answered ("Decisions of the user", 2026-11-22: "P1 Library 192 (Recommended)"; matrix cells T1, "Ja für alle" - zumindest solange bis die Test-Infrastruktur fertig umgebaut ist); task 2.1 builds on them. Question 2 (fractional exponent) stays open as written (nothing built for it).

## 1. Tests first (red)

- [x] 1.1 The red tests exist and are REPRODUCED on main: the matrix's `compiler-expr-1189-*` (24); `compiler-expr-1178-1189-*` (4) and `compiler-expr-1188-1189-db-B-01` are DIFFERS (one of their two fixes is in). Verify: `ctest --preset bug-orig -R compiler_expr_.*1189`.
- [x] 1.2 The bug-orig test `tests/bug-orig/compiler/compiler-pow-negative-base` (header `BUG(orig) (#1189)`) with the cases of design decision 3 that the matrix lacks, and the regression test `regression/compiler-pow-fraction-exponent` for a negative base with a fractional exponent (today's value, must not change). Verify: `.orig` recorded (`-DBUG_ORIG_RECORD_FILTER=compiler-pow-negative-base`, `--target bug-orig-record`); REPRODUCED; the regression test passes on main.
- [x] 1.3 The library capture: a call case of `mathtranslib`'s `^` entry with a negative base and an odd, an even and a fractional exponent in `libs/tests/systest_cmdlibs.c`; recorded with the original (`[orig]`). Verify: `libs_tests` pass on main with the new cases.

## 2. Implementation

- [x] 2.1 `mathtranslib`'s `^` entry as a routine (design decisions 1, 2), with the register contract of `libs/register-contract`; `blitzlib check` clean. Verify: the library builds without warnings; 1.1 and 1.2 FIXED after `; @fixed #1189`; the two-issue tests FIXED.
- [x] 2.2 The capture's `mathtranslib inline` entry 0 and the call cases of 1.3: `[orig]`/`[fixed #1189]` pairs (`libs-update`). Verify: `libs_tests` pass; `tools/results fix-scope` lists only these keys for the libraries.
- [x] 2.3 `BUG(orig) (#1189)` at `dopow` (`eval.c:1473`) becomes `Fixed (#1189)` naming the routine; a `Fixed (#1189)` comment at the routine. Verify: `grep -n 1189 programs/blitz2/src/eval.c libs/acidlibs/mathtranslib.c`.
- [x] 2.4 The matrix: `EM_R_1189` into `EM_RULES_PORTNOW`, `--target expr-matrix-update`, the fixed cells stay in their bug-orig tests with `; @fixed #1189` (decided: T1). Verify: `git diff` shows only #1189's cells; `ctest --preset host -R exprmatrix`; `ctest --preset bug-orig -R compiler_expr_.*1189` FIXED.

## 3. Checks

- [x] 3.1 `ctest --preset host`; `ctest --preset all -R 'corpus_exprmatrix|libs_tests'`; `corpus`, `corpus_runerr`. Verify: pass.
- [x] 3.2 Cost: a line for `^` (positive and negative base) in `tests/perf/bench` and `budgets.txt` with the CD library's target (`perf-original`); no existing budget rises. Verify: `ctest --preset perf`; the report gives the cycles per call for both bases and says whether the positive-base path costs more than the noise (D-007).
- [x] 3.3 Prev comparison: the `mathtranslib` library file and every executable with `^`, listed into `PREV_INTENDED` and the commit message. Verify: `ctest --preset all -R prev` with `-DPREV_INTENDED`.
- [x] 3.4 AB3 mode unchanged: `ctest --preset all -R '^ab3_cli'` and the AB3-mode CD corpus give the same executables (AB3 mode calls library 44). Verify: the prev comparison lists no AB3-mode result.
- [x] 3.5 `tools/results fix-scope` against `main`: only #1189's cases. Clean rebuild without warnings. Verify: the tool's report; the build log.

## 4. Documents

- [x] 4.1 `docs/libs/deviations.md` (kind-F row for #1189: the routine, the sign rule, the fractional exponent as before), `docs/blitz2/expression-types.md` (`^`), `docs/ab3/variant.md` (AmiBlitz3 v3.14.0 has the bug; AB3 mode keeps library 44's result). Verify: coordinator's review; links resolve.

## 5. Commit and report

- [x] 5.1 One fix commit: the routine, the tests, the regenerated matrix files, the capture pairs, the budget line, the documents, `Fixed (#1189)`, the changed results (`PREV_INTENDED`), `Fixes #1189`. Report: severity 1 proposed, topic "Numbers & dates"; open question 2 restated for the issue. Verify: `git show --stat`; the coordinator's fix-scope run.

## Workflow follow-up

- Coordinator: label, site entry and row, the acceptance runs after the merge (D-010), a comment on #1189 with the decided scope and the open runtime-error question (D-008, no internal references, D-022).
