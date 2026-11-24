# Proposal

## Why

The goal of #1180: the type of an operation depends on the wanted type and on both operands, not on their order. User, 2026-11-21 ("#1178 direction: variant C; expression tree planned"): "Und ja, base+q*4 muss das gleiche ergeben wie q*4+base, aber das machen wir in einem späteren Schritt". Requirement handed from #1178 (2026-11-22, "#1178 Q1"): "Zu #1178: Wichtig ist nur, dass nach dem Expression Tree dort 4.5 rauskommt" (`NPrint 1+1+1+q` with `q=1.5`; 4 until this change). After changes 1-9 the whole expression is a tree before emission; this change replaces the flow typing by operand typing. It is the named change of results of the series.

It also fixes #1183 (bug-orig, compiler, wrong-result; severity proposed 1): since #1177 a comparison compares a one-element left operand with its own value, but a compound left operand (`q*2`, `q+q`), a negated one (`-q`) and a Long constant on the left under a Float context are still evaluated in the type the expression is wanted in, so a Quick inside is cut and the boolean is wrong (`r.l=(q*2>2.5)` with `q=1.5`: 0, expected -1). The change `compare-compound-left` planned that fix on its own; its count (`~/.local/share/bb2-c-work/cmp-compound-1-1/report.md`) showed that the left operand's types are still decided in reading order, before the comparison operator is read, so the fix needs bottom-up typing, which is this change. User decision, 2026-11-24 (`rounds/tree/decisions.md`, "#1183 folded into step 5"): "Fold #1183 into step 5 (Recommended)"; the recorded order "#1180 steps 1-4, #1183, step 5" (`rounds/quick`, 2026-11-22) becomes "steps 1-4, step 5 with #1183". `compare-compound-left` is archived unimplemented (`openspec/changes/archive/2026-11-24-compare-compound-left`).

## What Changes

- **Readable tables of today's type rules first** (user, 2026-11-24): a C tool derives from `exprtype.c` tables per operator group (operator, left type, right type, wanted type -> result type and conversions, the exceptions as notes with their issues, constants and elements in their own table); they go into the docs with a build check that they equal the tool's output. The decision round shows each candidate rule as the same tables beside today's.

- **Operand typing**: a typing pass over the built tree; bottom-up each node gets the type its operands give (operator, left type, right type), top-down the wanted type; the result does not depend on the order of the operands of `+`, `*`, `AND`, `OR`, `EOR`, the comparisons, nor on the place of a subexpression in a chain.
- **The left operand of a comparison is typed by its own operands**, whatever its shape (compound, negated, a Long constant under a Float context), as the right operand since #1177: this follows from the operand typing; `compareop`/`cmplq` compare the two operand types exactly. `-q` already has its raw value (`quick-long-arithmetic`, design decision 7) and stays a scenario as a guard. The scope of the rule for the left operand is part of open question 1.
- The rule's details are decided before the implementation (open questions 1-3), with the tables of #1178 and a new run over its 376 places and the expression matrix's model.
- The bug-fix workflow where the user classifies the order dependence as an original bug (open question 4).
- The bug-fix workflow for #1183 (rule 4, spec `testing/bug-fix-workflow`): the BASIC model test `tests/bug-orig/compiler/compiler-compare-compound-left.test.bas/.toml` gets `fixed-orig = 1183` and the originals' values as `compiler=orig195` deviations (no `.expect`/`.orig`); the #1183 part of #1177's kind-F row in `docs/blitz2/deviations.md` becomes its own entry; `BUG(orig) (#1183)` (marker at `ownleft`, `eval.c`) becomes `Fixed (#1183)`; `Fixes #1183`.

## Not in scope

Optimisations the tree allows (#1180 step 6: folding beyond pairs, immediates, register saves); the comparison of strings and the boolean's type in the context (#1177's rule, unchanged); index typing (#1179); string operations.

## Results that change

Named (rule 3): the expression matrix cells whose expected value under the decided rule differs from today's (the list is produced by the model in task 2.2 and is the only set of matrix cells allowed to change); the bug-orig/fixed tests of the cells; `tests/blitz2/quick-arith-long.bas` lines 20/21 (`NPrint 1+1+1+q` 4 -> 4.5, `NPrint w+v+q` by the rule); the 376-place scan's changed places; the executables with a comparison whose left operand is compound, negated, or a Long constant under a Float context (the CD count of `compare-compound-left` found for the pure left-operand rule: 18 retyped comparisons in 7 programs, 0 executables changed, 0 values; the count is repeated for the decided rule); the 648 matrix cells with a whole-number constant on the left under Float (`df/pf-a/b-agree-01`: the code changes, the values not); the executables the prev run lists (named in `PREV_INTENDED` explicitly, not `ON`); `docs/blitz2/expression-types.md`, the bug-orig test `compiler-compare-compound-left` turning FIXED; the generated `docs/blitz2/expression-type-tables.md` (new, from task 1.2; regenerated with the decided rule), the site's "The type of an expression".

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `compiler/expression-types`: the requirement "The expected type flows as before" (from `quick-long-arithmetic`) is replaced by order independence (ADDED here; converted to MODIFIED/REMOVED in task 5.2). "Known limits of the left operand until the expression tree" is REMOVED and "No other result changes" MODIFIED (the #1183 exception goes); a requirement for the left operand of a comparison is ADDED.

## Impact

a new tool (`tools/lib/exprtype_tables/`, `tools/exprtables`, tests in `tools/tests/`) and the generated page `docs/blitz2/expression-type-tables.md` with its build check (first group); `exprtype.c` and its golden table, the typing pass in `exprtree.c`, `eval.c`; `tools/lib/exprmatrix/model.c` (the new rule), the matrix's generated tests (tests of the BASIC test model: their checks and declared deviations, `exprmatrix expect`), the #1178 tests; docs, site; `docs/ab3/variant.md` (AB3 mode by open question 2), the bug-orig test `compiler-compare-compound-left`, `docs/blitz2/deviations.md` (#1183's own entry). After `expr-tree-4b-lookaheads` (archived).

## Open questions (all Decide, before group 3; planning report Q4)

1. The rule: the type of Word+Word and Word*Word (today Long, so `w+v+q` differs from `w-v+q`); whether a constant takes the wanted type, the other operand's type, or its value class; how far a wanted whole-number type reaches into a Quick-only subtree; and the left operand of a comparison (scope: every compound left operand typed by its own operands, (a), or only where a Quick or Float is inside it or a Long constant meets a Float context, (b); `compare-compound-left`'s count: on the CD both change no executable, and (b) cannot be decided lexically, the types being known only during the build). Each candidate rule shows the values of that count: `r.l=(w+v>c)` w=v=20000 c=0 and `r.l=(w-(-v)>c)` (-1 under today's Long context, 0 when the Word sum wraps: a regression a whole-number rule must name or avoid), `r.l=(q*1000>50000)` q=100 (-1 -> 0: the Quick product overflows where the Long context computed exactly), `rq.q=(l1+1>40000)` l1=40000 (0 -> -1, a fix), `rf.f=(l1/3>3.2)` and `rq.q=(l1/3>3.2)` l1=10 (-1 -> 0: the whole-number division, as `If` does), `rb.b=(b1+b2>150)` (-1 both), `rf.f=(100000001>l3)` (the Long constant compared as a Long). Cycles measured there: `r.l=(w+v>c)` 95.5 -> 86.7, `r.l=(q*2>x)` 308.5 -> 426.7, `r.l=(-q<x)` identical, `rf.f=(100000001>l3)` 651.5 -> 510.1; the `If` forms are byte-identical (`adr.l=base+q*4`: Quick product with its +-32768 range, or exact then rounded as variant C). A candidate the decision round computes too (the user, 2026-11-24, `rounds/tree/decisions.md` "Constants as integral or fractional numbers"): a constant without a fraction is a generic integral number, one with a fraction a generic fractional number, and both are reinterpreted as needed once the types of both operands are known (`q.q=1.5 : l.l=q*2`: the 2 stays untyped until the tree decides a Quick-times-Long multiplication).
2. AB3 mode: the same rule (the port then differs from AmiBlitz3 in both modes; oracle tests marked) or flow typing kept in AB3 mode (two typings maintained). For #1178 the user chose "Neue Regel auch in AB3 (Recommended)" (another case).
3. Cost: code of changed expressions may be longer or shorter (Quick instead of Long operations); D-007 is recorded for library fixes; the measured cycles are shown with question 1.
4. Classification: one issue for the typing change (#1180 or a new bug-orig issue with the table of changed cases) or one issue per changed behaviour (rule 4: "one issue per bug"). #1183 stays its own issue (`Fixes #1183`, its own test and deviations entry).
