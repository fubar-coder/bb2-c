# Proposal

## Why

Issue #1183 (bug-orig, compiler, wrong-result; severity proposed 1): since #1177 a comparison compares a one-element left operand with its own value, but a compound left operand (`q*2`, `q+q`), a negated one (`-q`) and a Long constant on the left under a Float context are still evaluated in the type the expression is wanted in, so a Quick inside is cut and the boolean is wrong (`r.l=(q*2>2.5)` with `q=1.5`: 0, expected -1). The fix needs the operand types at the operator before code is emitted, which the expression tree gives after `expr-tree-4a-emit-after-build`. Recorded order: "#1180 steps 1-4, #1183, step 5" (`task-decisions.md` 2026-11-22 "Order of the Quick work; matrix test", "Go for steps 1-7"); "Compound left operand: "Ja, eigenes Bug-Issue (Recommended)"" (2026-11-21 "#1177 Float constant left; new bugs as issues"); "Constant left under Float: "Erstmal benannte Grenze, später korrigieren. Der Vergleich selbst soll - im Idealfall - verlustfrei sein."" (same entry); "`-q` left limit accepted": "Wichtig ist nur, dass er mit dem Expression Tree behoben ist." (2026-11-21).

## What Changes

- **The left operand of a comparison gets no wanted type**, whatever its shape, as the right operand since #1177: it is typed by its own operands (step 1's function at the comparison node, at build time), and `compareop`/`cmplq` compare the two operand types exactly. Scope of the rule (every compound left operand, or only those in which a value can change): open question 1.
- `-q` on the left: `quick-long-arithmetic` (design decision 7) already gives it its raw value, so `r.l=(-q<-1)` is right before this change; it stays a scenario of the spec as a guard. A Long constant on the left under a Float context is compared as a Long (new).
- The bug-fix workflow (rule 4, spec `testing/bug-fix-workflow`): `tests/bug-orig/compiler/compiler-compare-compound-left` (its inputs as `quick-long-arithmetic` task 1.4 left them, REPRODUCED before) gets `; @fixed #1183` and comes out FIXED; `BUG(orig) (#1183)` becomes `Fixed (#1183)`; the kind-F entry in `docs/blitz2/deviations.md`; `Fixes #1183`.

## Not in scope

Order independence of arithmetic (step 5, `expr-tree-5-operand-typing`); #1182 (merged before the tree); comparisons of strings; the boolean's type in the context (unchanged: #1177's rule).

## Results that change

Named (rule 3, `PREV_INTENDED` listed by the prev run): the executables with a comparison whose left operand is compound, negated, or a Long constant under a Float context (count: task 1.1, before the fix); the bug-orig test turning FIXED; the cells of the expression matrix that have such a left operand (expected none: the matrix has two-operand cells only; task 1.2 checks); `docs/blitz2/expression-types.md`'s limits section.

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `compiler/expression-types`: the requirement "Known limits of the left operand until the expression tree" (from `quick-compare-types`, scenario changed by `quick-long-arithmetic`) goes; a requirement for every left operand comes. Written as ADDED here because neither change is archived yet; converted to MODIFIED/REMOVED when they are (task 4.2).

## Impact

`eval.c` (`compareop`, `cmplq`, the comparison node), `exprtype.c`, the bug-orig test, docs and site (coordinator: site entry, overview row, label `fixed-in-port`). After `expr-tree-4b-lookaheads`, before `expr-tree-5-operand-typing`.

## Open questions

1. (Decide, after task 1.1's count) The rule's scope: (a) every compound left operand typed by its own operands (as the right operand), or (b) only where a Quick or Float is inside it or a Long constant meets a Float context. Assumption worked with: (a). Planning report Q3.
2. Cost (D-007 is recorded for library fixes, not for compiled comparisons): the cycles of the changed comparison code are measured in task 1.1 and shown with question 1.
