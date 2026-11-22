# Proposal

## Why

Under a Byte type so far a decimal literal with a fraction is cut to a whole number when it is read (`consttype2`, the `.byte` path: `makeint`, `programs/blitz2/src/libcall.c:438-447`; `blitz2.s:10938`), before the operation it takes part in; under Word, Long and Float it stays a Float (or a Quick). `rb.b=(1.5+1.5)` gives 2, `rb=(0.5*4)` 0, `rb=(1.5*vb)` 2 for `vb=2`, `rb=(2.5*2)` 4; under a Word destination 3, 2, 3, 5. blitz2 1.95, the CD's 2.10 and the port give the same; AmiBlitz3 v3.14.0 keeps the fraction (the matrix's class Q-F, 61 of its cells are #1188's). Issue #1188 (bug-orig, compiler, wrong-result; created by the coordinator by D-008/D-021 class (a)); bug-orig test `compiler-byte-fraction-const` and the matrix's open tests show it (REPRODUCED).

## Dependency and order

* After `compare-right-operand` (#1182) (recorded: "Go for steps 1-7", #1182 is step 5, "#1188, #1189, #1190" step 6; "changes that edit programs/blitz2/src/eval.c one after another"), and so after `quick-long-arithmetic` (#1178) and the #1179 change.
* Before `power-negative-base` (#1189) (planning choice, design decision 6): one test, `compiler-expr-1188-1189-db-B-01`, needs both fixes.
* After task 3.5 of `expression-type-matrix` (the AmiBlitz3 rows in `docs/ab3/variant.md` that this change removes or rewrites) and after the answer to `quick-long-arithmetic`'s open question 1 (where a fixed issue's matrix cells live).

## What Changes

- **A literal with a non-zero fraction keeps it under a Byte type so far**, as under Word: it is a Float constant, the operation is computed in Float and the result converted to the Byte at the end (towards zero, `SPFix`, as for every Float). `rb.b=(1.5+1.5)` 3, `rb=(0.5*4)` 2, `rb=(1.5*vb)` 3, `rb=(2.5*2)` 5.
- **A whole-valued literal written with a point or an exponent (`2.0`, `1e2`) stays a whole-number constant** under a Byte type so far, as today (design, decision 2: no slower code where the value is the same).
- **`Data.b` is not changed:** `Data.b 1.5` stays 1 (design, decision 3).
- **A new compile error** where the fixed literal meets an operator without a Float form, as under Word today: `rb=1.5 AND vb`, `OR`, `EOR`, the shifts, `rb=NOT 1.5` give "Illegal Operator for Type" (open question 1). The matrix already expects this (`compiler-expr-1188-refused-*`, 29 tests).
- **The #1186 re-emission of a cut literal on the left of a comparison** (`fraclitfloat`) is no longer reached for a literal with a fraction; it is removed or kept for the whole-valued literals by the prev comparison (design, decision 4).
- **Results change** (rule 3, `PREV_INTENDED`): the executables of programs with a fractional literal under a Byte type so far (a Byte destination, a Byte parameter, a Byte field or array element), whose expression becomes a Float computation; the matrix's #1188 cells (37+ cells in 4 open tests, 16 overflow cells of Q-A, the refused cells); `compiler-byte-fraction-const`.
- **AB3 mode:** the fix applies in both modes; AmiBlitz3 already keeps the fraction, so AB3 mode moves closer to AmiBlitz3 (open question 2).
- **Documentation:** `docs/blitz2/expression-types.md`, the kind-F entry in `docs/blitz2/deviations.md` (and the #1177/#1186 row's sentence on the Byte literal), `docs/ab3/variant.md` (the #1188 rows of task 3.5 of the matrix go), `BUG(orig) (#1188)` → `Fixed (#1188)` in `consttype2`.

## Not in scope

- The rounding of the Float result (`SPFix`, towards zero), overflow of the Byte result (the matrix's baseline, Q-A).
- `Data.b`, `Data.w` with fractions (decision 3).
- Quick, Word, Long and Float type so far (unchanged); a fraction constant in a comparison (fixed by #1186).
- `^` (#1189), the matrix's other open issues.

## Open questions

1. **Bit operators with the fixed literal** (`rb=1.5 AND vb`, `NOT 1.5`, the shifts): refused, as under Word (recommended; the matrix's tests expect it), or the cut kept for these operators only. The option text of the matrix question Q-C named the refusal, but the user's correction of 2026-11-22 makes Q-C's answer a baseline, not a rule, so it is not a decision on this. Asked together with `compare-right-operand`'s question 1. Worked with: refused.
2. **AB3 mode:** both modes (recommended; matches AmiBlitz3). #1178's AB3 answer was for #1178; for this case the user's. Worked with: both modes.
3. **Where the matrix's #1188 cells live after the fix:** the same choice as `quick-long-arithmetic`'s open question 1 (T1 the bug-orig tests get `; @fixed #1188`; T2 corpus pairs; T3 both). Worked with: the answer given for #1178, if the user gives it for every matrix issue; otherwise asked for this case.

## Capabilities

### New Capabilities

- `compiler/expression-types`: a literal with a fraction under a Byte type so far. (Created by whichever of `quick-compare-types`, `quick-long-arithmetic`, `compare-right-operand` and this change is archived first.)

### Modified Capabilities

(none in `openspec/specs/`)

## Impact

- `programs/blitz2/src/libcall.c` (`consttype2`, `fetchnum3`, the data flag, the #1186 helpers `litcut`/`fraclitfloat`), `libcall.h`, `programs/blitz2/src/eval.c` (`cmpownleft`'s call of `fraclitfloat`, if decision 4 removes it); shared by `blitz2-ab3`.
- Tests: `tests/bug-orig/compiler/compiler-byte-fraction-const`, the matrix's `compiler-expr-1188-*` (4 value tests, 29 refused tests) and `compiler-expr-1188-1189-db-B-01`, the regeneration by `expr-matrix-update` with `EM_R_1188` in the port's rule set (`tools/lib/exprmatrix/exprmatrix.h`, `EM_RULES_PORTNOW`), `regression/compiler-compare-fraction-const` (must stay unchanged), the prev comparison.
- Docs as listed; site entry and overview row (coordinator).
- Related: #1186, #1177, #1189.
