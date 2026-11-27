# Proposal: type-system-rework

## Why

The type of an operation in blitz2 depends today on the destination, on the
order of the operands and on whether an operand is a constant or a variable
(the original's "type so far", reworked step by step by #1178, #1216 (rule
P+), #1222-#1228). The results surprise: `x.l=1-1.25/3` is 0 but
`x=1-q/3` with `q=1.25` is 1, `NPrint qa+3 LSR 3` is 1.625 but with
variables 1.25, `NPrint i*300` is 24464 but `r.l=i*300` 90000. The user
decided on 2026-11-27 to rework the whole type system (rounds/tree,
"Rework of the whole type system (direction)" through "Type system rework,
round 5: LSR vs LSL in the shift exception"): the value of an expression is
the same whether it is folded or computed with variables, and the
destination has no influence on the computation except the final conversion
(named exceptions near the top of an expression: a shift under a Quick
destination, a whole `/` under a Quick or Float destination, the top
operation under a Float destination, and a whole `/` beside a Quick; the
user's Q12 rule: as close to the original as possible on the example
programs). These exceptions are a closed list, and each place where one
changes a type gives a compiler warning ("type-system-rework conflict item
29", "... I9 and exception warnings").

## What Changes

- **BREAKING (results of compiled programs):** every operation is typed from
  its operands only: whole operations one step wider than the wider operand
  (Byte -> Word -> Long, capped at Long), an operation with a Quick in
  48.16, with a Float in Float. The destination (or parameter type) only
  converts the result.
- **BREAKING:** `/` of two whole operands is whole, towards zero, except
  under a Quick or Float destination at the top or reached through
  `+ - * /` and unary minus (`x.q=1/16` 0.0625, `x.q=15-(w/16)` 7.6875,
  `x.q=(a/b)/c` 1.75) and beside a Quick-class operand (`NPrint q+w/2` 2;
  not beside a Float); `x.q=(7/2=3)` is -1.
- **BREAKING:** the conversion of a Quick to a whole destination rounds down
  (`x.l=-1.5` is -2, the constant being an exact Quick), of a Float towards
  zero as before (`x.l=f` with `f=-2.5` is -2), beyond the Long range
  wrapped (`x.l=1e10` 1410065408); Byte, Word and Long wrap, a Quick
  saturates (`x.q=b+100000` is 32767.99998); `Data.b -1.5` is -1.
- Whole constants take their smallest type (no DEFTYPE influence); a constant
  with a point is a Quick when it is exact in 16.16 or beside a Quick, else a
  Float, never whole (fixes #1231: `NPrint 3/2.0` prints 1.5).
- `Mod` is floor Mod (the remainder has the divisor's sign), exact with
  fractions (closes #1197 and #1198).
- Bit operators take whole operands (a Quick rounded down), the result is
  whole. A Quick is shifted with its fraction also under a whole destination
  (`r.l=q LSL 2` with `q=1.5` is 6); a whole number is shifted as a whole
  number, right shifts in its own width, left shifts as a Long
  (`NPrint qa+3 LSR 3` is 1.25, `1 LSL 17` 131072). Under a Quick
  destination a shift at the top or reached through `+ -` keeps the
  original's form (`x.q=1 LSR 7` 1/128, `x.q=qa+3 LSR 3` 1.625); a left
  shift that is the whole expression wraps (`x.q=1 LSL 16` 0, also of a
  Quick), one inside a sum saturates.
- New compiler warnings, one name per exception: `divfrac`, `divquick`,
  `floattop`, `shiftfrac`, `shiftwrap`, and `longwrap` (a Float beyond the
  Long range known at compile time), `datafrac` (a Data.b value loses a
  fraction); in the editor at most 8 per name and compile, on the console
  every place.
- Without a destination (Print, untyped parameters) a 48.16 result is a
  Quick when it fits, else a Long when whole, else a Float; comparisons,
  `If` and `Select` compare the full value.
- Constant folding gives the variable form's value for every operator and
  destination (folds #1225, #1229, #1230, #1233 and the constant shift
  beside a Quick into the change).
- The expression-matrix model gets the new rules as its oracle (#1232);
  the matrix, the docs, the site pages and `docs/blitz2/deviations.md` are
  rewritten for the new rules.
- AB3 mode follows the same rules; the differences from AmiBlitz3 get rows in
  `docs/ab3/variant.md`.

Not in scope: Float arithmetic itself (FFP stays as it is), the string
operators, `Int`/`Frac`/`Abs` and other functions (they take their argument
by its type), the expression limits (`compiler/expression-limits`), the
array index range checks, new operators or types, the 68020+ code.

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `compiler/expression-types`: the typing of every operator is replaced by
  the operand-only rules (width, constants, whole division, conversion, bit
  operators, shifts and the shift exception, no destination, comparisons,
  folding = variable form, AB3 mode); the requirements of #1178, #1216 and
  #1222 that depend on the destination are removed; the requirements whose
  values change are modified.
- `compiler/warnings`: the warnings of the type exceptions, `longwrap` and
  `datafrac`, and how many of them are shown.

## Impact

- Code: `programs/blitz2/src/exprtyping.c` (the typing pass), `exprfold.c`,
  `exprarith.c`, `eval.c` (code generation, the fast paths, p5consteq /
  p5constapply / p5qval), `libcall.c` (`quickconst`, the constant reading),
  `libs/compilerlib/compilerlib.c` (saturating 48.16 -> Quick conversion,
  the no-destination conversion, Float -> whole beyond the Long range
  wrapped; D-027); the warnings through `ext_warning` (`parse.c`).
- Test runners: a command-line compile with an exception place ends with
  return code 5 (warnings enabled by default); the runners that compile
  corpus, CD and matrix sources handle it.
- Model and matrix: `tools/lib/exprmatrix/model.c` (a new rule set, the
  oracle), `tests/corpus/tests-exprmatrix/*`, the generated
  `tests/bug-orig/exprmatrix/` and `tests/bug-orig/compiler/compiler-expr-*`
  sources, `tests/ab3/.../exprmatrix` (after ab3-test-model group 7).
- Results (CLAUDE.md rule 3): the executables of every corpus, CD and test
  program with Quick arithmetic, Float -> whole conversions or whole
  divisions under a Quick/Float destination change; all are intended and
  named with `-DPREV_INTENDED`. Measured on the CD dump with the decided
  rules and the adopted Q12 classes: 468 places change for some value set
  (83 at non-negative values without overflow); 188 places move away from
  1.95 (74 at non-negative values), mostly the 48.16 cut by one 1/65536
  step; 2,233 of 30,591 evaluated matrix cells change.
- Speed: Quick arithmetic under a Quick destination becomes 48.16 with a
  saturating conversion (estimated +30 cycles for `+`/`-`, +140 for `*`;
  `tests/perf/budgets.txt` lines named in the tasks).
- Docs: `docs/blitz2/expression-types.md`, `expression-type-tables.md`,
  `deviations.md`, `docs/ab3/variant.md`, the site pages
  `language/expressions.md` and `language/types.md`, `TODO.md`.
- Issues: #1197, #1198, #1225, #1229, #1230, #1232, #1233 and #1236 (the
  constant shift beside a Quick) are closed by the change; #1231 (bug-orig)
  by its fix workflow; #1235 "[blitz2] The operand-only type system"
  (enhancement) tracks the change, its tests `extension = 1235` (Q19).
- Open changes sharing files: `ab3-test-model` (the exprmatrix generator,
  tests/ab3 matrix, tests/README.md), `ab3-mode-g5` (`docs/ab3/variant.md`,
  `docs/blitz2/deviations.md`, tests/README.md), `compiler-function-optimizer`
  (`docs/ab3/variant.md`); the rework starts after ab3-test-model is merged
  and before ab3-mode-g5's docs tasks (Q18).
