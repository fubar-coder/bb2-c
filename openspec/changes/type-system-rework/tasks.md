# Tasks: type-system-rework

Every commit is one topic; every commit that changes results runs the prev
comparison of the affected programs and names each changed result with
`-DPREV_INTENDED` and in the message (CLAUDE.md rule 3). Agents run their
tests, the affected registrations and `ctest --preset host` (D-026); the
full acceptance is the coordinator's. The work starts after the change
ab3-test-model (including its group 7) is merged and before ab3-mode-g5's
docs tasks ("type-system-rework Q11, Q16, Q18, Q19, Q20", Q18).

## 0. Gate

- [x] 0.1 The user's answers to Q1-Q20 are recorded by the coordinator and folded into the change (spec delta, design.md "Answers to the planning questions"); `openspec validate type-system-rework --strict` passes.
- [x] 0.2 The user's answers to the Q12 classes (A, B, C1, C3, D, E, J adopted; F not) and to N1, N2 are recorded and written into the spec delta as named exceptions; the measurements repeated with all of them (`scripts/cdrun3.py`; design.md "Measurements").
- [x] 0.4 The user's answers to the interaction questions I1-I9 (design.md, "Open questions") are recorded and folded in with `openspec-update-change`. Verify: `openspec validate type-system-rework --strict`; the spec scenarios that depend on them updated.
- [x] 0.3 The coordinator files the issue "[blitz2] the operand-only type system" (enhancement, compiler; Q19) and the bug-port issue "a constant shift beside a Quick is folded as a 16.16 shift" (shift-print/report.md, option A). Verify: both numbers written into this file and design.md. Filed: #1235 (the type system), #1236 (the constant shift).

## 1. The model as the oracle (#1232)

- [x] 1.1 `tools/lib/exprmatrix/model.c`: a rule set `EM_RULES_TSR` beside the P+ sets and `EM_RULES_ORIG` (Q11), written from the spec delta only (not from exprtyping.c): classes from the operands, the conversions, the shift exception, the no-destination form, folding = variable form; each rule names its decision by heading and date. Verify: `tools/tests/test_exprmatrix_model.cpp` gets one case per spec scenario (values as in the spec) and the #1232 shapes; it fails on a broken rule (a case with a deliberately wrong expectation in the test's broken variant); `ctest --preset host`.
- [x] 1.2 The model does not crash on `(1.25/1000)^7` under Byte, Word, Long (#1232). Verify: a host test case.
- [x] 1.3 Re-measure with the model: `exprmatrix candidates` with the new column over the matrix and the CD dump (`cd.cases`); compare with design.md "Measurements" (cells and places per cause). Verify: the numbers written into design.md; a difference above 1 % of a cause is reported to the coordinator before 3.x starts.
- [ ] 1.5 The model follows the answers D1-D7 (rounds/tree "2026-11-28 type-system-rework D2, D3, D4", "... D1/D7 ..., D5", "... D6") and the planner's P1 (a point constant read as a Quick through FFP); its P+ and orig columns fixed by the emulator values of section1-decisions.md (#1232: a Float division by 0 traps, a Long division by 0 gives +2147483647 for every sign, the Quick literal through FFP); 1.3 recounted with "crash -> value" apart from "away from 1.95". Verify: a host case per answer and per fixed column value; the numbers in design.md "Measurements"; a cause that still differs by more than 1 % is reported before 3.x starts.
- [ ] 1.4 The nesting depth of the CD's expressions under the new rule (every Quick operation a register pair; method of expr-tree-5 3.7). Verify: no CD source reaches "Expression too Complex"; the count in design.md.

## 2. Tests of the new rules (they fail until section 3/4)

- [ ] 2.1 `tests/blitz2/expr-tsr-*.test.bas`/`.toml`, one per spec requirement of the delta, with the spec's values (including the I1, I4, I8 and D1-D7 scenarios, a real divisor 0 of every type and of a quotient reached by C1 under a Quick and under a Float (the trap), and each I4 form with variables and with constants); `extension = 1235` (Q19), the deviations 1.95's values. Verify: they come out as the expected failures on main (listed in the commit), compile on both sides.
- [ ] 2.2 #1231 before its fix: a bug-orig test `tests/bug-orig/compiler/compiler-point-whole-div` (`NPrint 3/2.0` 1.5, `NPrint 7/2.0+0.5` 4) with its `.orig` from `bug-orig-record`; the comment `BUG(orig) (#1231)` at `const_fetch`. Verify: `ctest --preset bug-orig -R point-whole-div` REPRODUCED.

## 3. Typing pass and folding

- [ ] 3.1 `exprtyping.c`: every node typed from its operands (whole width one step wider, 48.16 with a Quick, Float with a Float); the wanted type only at the root and for the top shift under a Quick; `const_flow`, the wanted-type cases of `typed()`, `has_long`, `frac_side`, A1 and the cut by direction removed; `exprtyping.h` documentation. Verify: `test_exprtyping.cpp` against the model's rule set for all matrix cells and the CD expressions (the host test of expr-tree-5 task 3.3, retargeted); `ctest --preset host`.
- [ ] 3.2 Constants: a whole constant its smallest type without DEFTYPE influence (a minus before a constant the unary minus, a Word for `-8`: N2); a constant with a point a Quick only when exact in 16.16 or beside a Quick, else a Float (Q8); the named exceptions of the typing: a whole `/` at the top or through `+ - * /` and unary minus under a Quick or Float destination with its fraction, a Float quotient under a Float (Q9, C1, I3, I4), a whole `/` beside a Quick-class operand with its fraction, not beside a Float (C3, I1), the top operation under a Float destination in Float with a point constant read as a Float (D, I2). Verify: the expr-tsr constant tests; prev comparison named.
- [ ] 3.3 Fix #1231 by the bug-fix workflow (docs/development.md, "Fixing an original bug"): `fixed-orig = 1231`, `Fixed (#1231)`, the kind-F entry in `docs/blitz2/deviations.md`, the changed results named, `Fixes #1231`; fix-scope run. Verify: the test of 2.2 FIXED; fix-scope.
- [ ] 3.5 Warnings (compiler/warnings delta): `divfrac`, `divquick`, `floattop`, `shiftfrac`, `shiftwrap` at every place where an exception changes the type, `longwrap` for a wrap known at compile time, `datafrac` for a Data.b value with a nonzero fraction; at most 8 per name and compile in the editor plus `NAME: N more`, every place on the console; both modes. Verify: one test per warning (its line and name), one with no exception, the editor limit with 10 places, a check with deliberately broken input; the executables unchanged with the warnings disabled.
- [ ] 3.6 Test runners: a command-line compile with only these warnings ends with return code 5; the runners that compile corpus, CD, matrix and site sources handle it (warnings disabled for them or RETURN_WARN accepted; the choice named in the report). Verify: `ctest --preset host`, the corpus and matrix registrations, `originals` on a sample.
- [ ] 3.4 Folding = variable form: `exprfold.c`/`exprarith.c` compute a constant subtree with the same rule table (whole int32 wrap, 48.16 int64 with the cut of Q1, FFP fold as today); `const_value`'s shifts as the 68000 (#1233); `p5consteq`/`p5constapply` compare the value; tests `fixed = N` for #1225, #1229, #1230, #1233 and #1236, one `Fixes` line each. Verify: the issues' cases on both sides; a host test that folds every matrix cell's constant form and compares it with the variable form under the model.

## 4. Code generation

- [ ] 4.1 Whole operations one step wider: `ext` + Long code where the value needs it; the narrow code where design.md "Optimizations" allows it (wrapping destination of n bits, `+ - *` and bit operators, or a range bound). Verify: expr-tsr width tests; the array-index code-size requirement ("The index code of a plain later index does not grow") still passes; prev comparison named.
- [ ] 4.2 Every Quick operation in 48.16; the 16.16 code only where value-identical (one `+`/`-` with `BVS` to the saturation under a Quick, a whole-valued fast path that returns the exact value); the less exact fast paths changed or removed by Q20. Verify: expr-q48-paths and expr-q48-cut-down rewritten to the new values (P3), bq48/bmath perf loops run.
- [ ] 4.3 Conversions in compilerlib (D-027): 48.16 -> Quick saturating, Float -> whole towards zero as SPFix (B; today's routine), Float -> Quick rounding down and saturating, a Float beyond the Long range cut towards zero, then wrapped (I8); their host tests in `tools/tests` against `exprarith`. Verify: compilerlib host tests; `bcompilerlib` perf loops; prev comparison names every program that links compilerlib (D-027).
- [ ] 4.4 Shifts: a Quick's `LSL`/`ASL`/`ASR` in 48.16 and `LSR` on the 16.16 bits (Q15); a whole operand's `LSR`/`ASR` in its own width and `LSL`/`ASL` as a Long (A); a Quick count rounded down without making the left operand fractional; the shift exception under a Quick destination at the top or through `+ -` (E): LSR/ASR on the 16.16 bits, in 48.16 beyond the Quick range (Q13, N1); LSL/ASL as a Long; a left shift that is the whole expression, of a whole or a Quick operand, wraps at the conversion (J, I6), one inside a `+ -` sum saturates (I5). Verify: expr-tsr shift tests; `rb.b=1.5 LSL 2` 6; CD shift idiom `hsp=1 LSR 7`.
- [ ] 4.5 No destination: Print, NPrint, Str$ and "any" parameters (P1) pass a 48.16 value as a Quick, a Long or a Float decided at run time; a whole value as itself. Verify: expr-tsr print tests; `compiler-operand-order-print` rewritten.
- [ ] 4.6 Comparisons, If, While, Until, Select/Case compare the full value; the boolean a Word -1/0 (Q5). Verify: compiler-operand-order, compiler-compare-compound-left, select-fraction tests; the existing exact Long/Quick compare unchanged (bcmp perf).
- [ ] 4.7 Division and Mod by a run-time 0: the division trap for every type (D1/D7), a constant 0 the compile error as before; and floor Mod with fractions (Q2); the `Mod` tests of #1197 and #1198 rewritten to the floor values with `Fixes #1197` and `Fixes #1198` in the commit (Q2: they close with the rework). Verify: expr-tsr div0/mod tests; array-index-div0 and divzero-next-compile checked, rewritten where they rely on $7FFFFFFF or the dividend (named in the commit).
- [ ] 4.8 AB3 mode: the same rules in blitz2-ab3; the differences from AmiBlitz3 v3.14.0 as rows in `docs/ab3/variant.md`. Verify: the AB3 cases of the expr-tsr tests; the ab3 matrix cases after 6.2.

## 5. Speed

- [ ] 5.1 Measure the perf loops (`ctest --preset perf`, bmath, bq48, bcompilerlib, bcmp); add loops for `q3=q1+q2`, `q3=q1*q2` under a Quick, `NPrint w+v` and the nested divisions of I4 (`x.q=(a/b)/c`, `x.f=(a/b)/c`, A500 cycles, D-007); raise only the budgets the user accepted (Q16), each named in the commit. Verify: the perf preset passes; the table in design.md "Cost" replaced by the measured values.

## 6. The matrix

- [ ] 6.1 `expr-matrix-update` with the new rule set; the cells changed against the previous expectations classified by the rework's issue; `named-cells.tsv` and `left-out.txt` regenerated; the `compiler-expr-1216-*` tests replaced by the classes of the rework. Verify: `exprmatrix_committed`; the matrix tests on both sides; the count of changed cells against 1.3.
- [ ] 6.2 The AB3 matrix cases (after ab3-test-model group 7, Q18) regenerated. Verify: the ab3 matrix tests on both sides; REF_EXPECT where AmiBlitz3 differs.

## 7. Docs and site

- [ ] 7.1 `docs/blitz2/expression-types.md` rewritten for the rules (the original's rule kept as the comparison), `expression-type-tables.md` regenerated, "Where the rules are in the code" updated. Verify: every requirement of the delta has its section; no statement of the conflict list left.
- [ ] 7.2 `docs/blitz2/deviations.md`: the rows of #1178, #1186, #1188, #1216, #1221-#1228 replaced by the rework's rows (kind F for #1231; the rework's rows by Q19). Verify: the build check of the deviations table; links.
- [ ] 7.3 Site: `language/expressions.md` ("The type of an expression", "Some details of the operators", the `MOD` sentence settled by Q2), `language/types.md` (conversion, number constants), `compared-with-original/expressions/type-system`, `how-blitz2-works/compiling`. Verify: the site build; the examples compile (site-examples tests).
- [ ] 7.4 `TODO.md`: the `x/2*2` entry (whole operands) and the open type items updated; `tests/README.md` for the new tests and the matrix classes. Verify: review of the diff.

## 8. Verification

- [ ] 8.0 The P+ rule sets (`EM_RULES_DECIDED`, `EM_RULES_P`, the candidate sets) removed from `model.c` with their host tests, `EM_RULES_ORIG` and `EM_RULES_TSR` kept (Q11). Verify: `ctest --preset host`; `exprmatrix_committed`.

- [ ] 8.1 The agent's checks: the tests of sections 2-6, the affected registrations, the prev comparison of the affected programs with every changed result named, `ctest --preset host` with and without `-DHOST_SANITIZE=ON`, a clean rebuild without warnings. Verify: the logs' summaries in the report.
- [ ] 8.2 The CD value check repeated with the port (the typed dump on the emulator for one value set per changed place of design.md). Verify: every changed place is one of the listed causes.

## Workflow follow-up

- The coordinator: fix-scope again for #1231, the targeted checks after each merge, the full acceptance (`all`, `bug-orig`, `originals`, clean rebuild) before the push (D-026).
- The website rows and the fix-overview row of #1231, the labels, closing #1197, #1198, #1225, #1229, #1230, #1232, #1233, #1235 and #1236 after the push: the coordinator.
- Archive the change after the merge (`openspec-archive-change`).
