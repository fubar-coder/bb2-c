# Proposal

## Why

Fourth change of the expression-tree series (#1180 step 3; the series: `expr-tree-1-typing/design.md`). Arrays, pointers and paths are parsed and compiled in one routine, `calcvar` (`parse.c`), which compiles every index with `arreval`/`arrevalchk` while it reads the variable. The analysis rates it the largest and riskiest element (many paths: list arrays, `\` paths, `UsePath`, NewType arrays, pointers; few of them in the corpus). It gets its own change.

## What Changes

- `calcvar` split into a parse part that returns a variable node (the variable, its path of fields, its index subtrees, the bound checks wanted) and an emit part; the indexes are subtrees built by the evaluator's builder (`arreval`, `arrevalchk` with #1179's rule as merged).
- Emission at today's moments (window): the index code where `calcvar` emits it today, including the runtime checks (`arrszchk`, the bound checks, `overchk`).
- The side-effect table (`docs/blitz2/expression-tree.md`) for these kinds.

## Not in scope

The left side of an assignment as a whole (`dolet2`'s `pushdo`/`pushput`: `expr-tree-4b-lookaheads`), the index typing itself (#1179, merged before), deferred emission.

## Results that change

None (rule 3); the check of the series, with the runtime-error passes (`corpus_exprmatrix_runerr_*`, `corpus_runerr`) in particular.

## Capabilities

### New Capabilities

(none; `skip_specs`)

### Modified Capabilities

(none)

## Impact

`parse.c` (`calcvar` and its helpers), `eval.c` (`arreval`, `arrevalchk`, `variable`), `tokens2.c` only where `calcvar`'s callers in statements pass through, `exprtree.c`, docs. After `expr-tree-3a-leaves`. Overlap: `ab3-mode-g5` edits `parse.c` (`notypeerr` paths, not `calcvar`): no order needed (planning choice).
