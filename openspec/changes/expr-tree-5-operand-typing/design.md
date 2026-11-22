# Design

## Context

After `compare-compound-left` the expression is a typed tree before emission (flow typing at build time, `expr-tree-4a-emit-after-build`); step 1's function decides each operation from (operator, left, right, facts). Variant C of #1178 and its rounding ("Zwischenlösung: / gegen Null (Recommended)") are the value rules of a mixed operation. The series: `expr-tree-1-typing/design.md`.

## Goals / Non-Goals

**Goals:** typing by operands; the changed results listed exactly by the model before the implementation.

**Non-Goals:** step 6 optimisations.

## Decisions

### 1. Typing pass after the build, errors still at build time

The flow typing at build time stays for the errors (the first of two errors, spec `compiler/expression-limits`); a second pass over the finished tree (iterative along chains) computes the operand types and the conversions; emission uses them. Where operand typing would make a source that compiled refuse (or the other way), that is a named change of its own (task 1.2 lists such cases; expected none).

### 2. The model first

The expression matrix's model (`tools/lib/exprmatrix/model.c`) gets the decided rule as a third rule set; the cells whose value changes are the named cells. The 376 places of #1178 are re-run with the same rule. The implementation is done when the compiler gives the model's values for every cell.

## Risks / Trade-offs

- [The changed set is larger than predicted] -> the matrix's named-cell list is the limit: a cell not on it that changes fails the matrix pass.
- [The rule is chosen without seeing the CD impact] -> the 376-place run and a CD count are shown with question 1.
- An option that changes results not intended and named (e.g. accepting `PREV_INTENDED=ON` without a list) would be I5 Critical; not proposed: the prev run's list is named.
