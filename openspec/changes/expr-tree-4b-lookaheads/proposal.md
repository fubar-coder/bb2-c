# Proposal

## Why

Ninth change of the expression-tree series (#1180 step 4, second part; the series: `expr-tree-1-typing/design.md`). With the whole expression built before emission (`expr-tree-4a-emit-after-build`), the remaining single-pass mechanisms have their information in the tree: `countpees` rescans a parameter list in the source to count its commas (`asmlibs.c`; quadratic in the nesting of calls); the intrinsics read the raw text behind the cursor (`fracend`, `intrstrvar`) and count elements (`frachere`, `elemcount`, `letelem`) and thread a state machine through four routines; `dolet2` and `doselect` compile the address of the left side, copy it away and copy it back behind the right side (`pushstart`/`pushdo`/`pushput`, `libcall.c`).

## What Changes

- `countpees` replaced by the call node's parameter count; the source is no longer rescanned.
- The intrinsics decided from node properties ("the argument is a Quick", "this call is the whole right side"); the state machine and the text look-aheads go.
- `dolet2` and `doselect`: the left side's address and the right side built as trees and emitted in today's order (right side, then the address code where today's copy puts it); the copy buffer goes.

## Not in scope

Any type change (#1183, step 5); register saving or immediates (step 6, not planned); `evalconst2` (`#c=`, `Dim`, `CNIF`: compiled into a side buffer and run, stays).

## Results that change

None (rule 3); the check of the series.

## Capabilities

### New Capabilities

(none; `skip_specs`)

### Modified Capabilities

(none)

## Impact

`asmlibs.c` (`countpees`), `eval.c` (the intrinsics, `libfunction`, `getap`, `maineval2`), `compile.c` (`dolibtoke`'s `intrinsic_code`), `tokens2.c` (`dolet2`), `tokens1.c` (`doselect`), `libcall.c` (`pushstart`, `pushdo`, `pushput`). After `expr-tree-4a-emit-after-build`. Overlaps: `ab3-mode-g5` edits `tokens2.c` (`doend3`, `stateret`, `funcret`) and `asmlibs.c`; `buffer-access-annotations` names `tokens1.c`; different functions, no order set (planning choice).
