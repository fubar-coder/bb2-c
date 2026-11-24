# Proposal

## Why

The goal of #1180: the type of an operation depends on the wanted type and on both operands, not on their order. User, 2026-11-21 ("#1178 direction: variant C; expression tree planned"): "Und ja, base+q*4 muss das gleiche ergeben wie q*4+base, aber das machen wir in einem späteren Schritt". Requirement handed from #1178 (2026-11-22, "#1178 Q1"): "Zu #1178: Wichtig ist nur, dass nach dem Expression Tree dort 4.5 rauskommt" (`NPrint 1+1+1+q` with `q=1.5`; 4 until this change). After changes 1-9 and #1183 the whole expression is a tree before emission; this change replaces the flow typing by operand typing. It is the named change of results of the series.

## What Changes

- **Operand typing**: a typing pass over the built tree; bottom-up each node gets the type its operands give (operator, left type, right type), top-down the wanted type; the result does not depend on the order of the operands of `+`, `*`, `AND`, `OR`, `EOR`, the comparisons, nor on the place of a subexpression in a chain.
- The rule's details are decided before the implementation (open questions 1-3), with the tables of #1178 and a new run over its 376 places and the expression matrix's model.
- The bug-fix workflow where the user classifies the order dependence as an original bug (open question 4).

## Not in scope

Optimisations the tree allows (#1180 step 6: folding beyond pairs, immediates, register saves); the comparison rule (#1177, #1183 done); index typing (#1179); string operations.

## Results that change

Named (rule 3): the expression matrix cells whose expected value under the decided rule differs from today's (the list is produced by the model in task 1.2 and is the only set of matrix cells allowed to change); the bug-orig/fixed tests of the cells; `tests/blitz2/quick-arith-long.bas` lines 20/21 (`NPrint 1+1+1+q` 4 -> 4.5, `NPrint w+v+q` by the rule); the 376-place scan's changed places; the executables the prev run lists (named in `PREV_INTENDED` explicitly, not `ON`); `docs/blitz2/expression-types.md`, the site's "The type of an expression".

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `compiler/expression-types`: the requirement "The expected type flows as before" (from `quick-long-arithmetic`) is replaced by order independence. ADDED here while neither change is archived; converted to MODIFIED/REMOVED when they are (task 4.2).

## Impact

`exprtype.c` and its golden table, the typing pass in `exprtree.c`, `eval.c`; `tools/lib/exprmatrix/model.c` (the new rule), the matrix's expected files, the #1178 tests; docs, site; `docs/ab3/variant.md` (AB3 mode by open question 2). After `compare-compound-left`.

## Open questions (all Decide, before group 2; planning report Q4)

1. The rule: the type of Word+Word and Word*Word (today Long, so `w+v+q` differs from `w-v+q`); whether a constant takes the wanted type, the other operand's type, or its value class; how far a wanted whole-number type reaches into a Quick-only subtree (`adr.l=base+q*4`: Quick product with its +-32768 range, or exact then rounded as variant C). A candidate the decision round computes too (the user, 2026-11-24, `rounds/tree/decisions.md` "Constants as integral or fractional numbers"): a constant without a fraction is a generic integral number, one with a fraction a generic fractional number, and both are reinterpreted as needed once the types of both operands are known (`q.q=1.5 : l.l=q*2`: the 2 stays untyped until the tree decides a Quick-times-Long multiplication).
2. AB3 mode: the same rule (the port then differs from AmiBlitz3 in both modes; oracle tests marked) or flow typing kept in AB3 mode (two typings maintained). For #1178 the user chose "Neue Regel auch in AB3 (Recommended)" (another case).
3. Cost: code of changed expressions may be longer or shorter (Quick instead of Long operations); D-007 is recorded for library fixes; the measured cycles are shown with question 1.
4. Classification: one issue for the typing change (#1180 or a new bug-orig issue with the table of changed cases) or one issue per changed behaviour (rule 4: "one issue per bug").
