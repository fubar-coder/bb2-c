# Proposal

## Why

Ninth change of the expression-tree series (#1180 step 4, second part; the series: `openspec/changes/archive/2026-11-24-expr-tree-1-typing/design.md`). With the whole expression built before emission (`expr-tree-4a-emit-after-build`), the remaining single-pass mechanisms have their information in the tree: `countpees` rescans a parameter list in the source to count its commas (`asmlibs.c`; quadratic in the nesting of calls); the intrinsics read the raw text behind the cursor (`fracend`, `intrstrvar`) and count elements (`frachere`, `elemcount`, `letelem`) and thread a state machine through four routines; `dolet2` and `doselect` compile the address of the left side, copy it away and copy it back behind the right side (`pushstart`/`pushdo`/`pushput`, `libcall.c`).; `arrevalchk` narrows a later array index by patching the code already written for it.

## What Changes

- `doselect`: the expression written behind its `clr` instead of copied (same bytes). `dolet2`: the right side emitted before the left side's address code, the copy buffer gone; this changes the executables of assignments whose left side has code and is the fix of #1212 (its own fix commit).
- `arrevalchk`'s narrowing of a later array index decided on the node; the patch of written code goes.

## Not in scope

`countpees` (the parameter count chooses the form before the parameters are read) and the intrinsics' state machine and look-aheads (`Len`/`Asc` decide before their argument is built): both stay (design, decisions 1 and 2). Any type change (#1183, step 5); register saving or immediates (step 6, not planned); `evalconst2` (`#c=`, `Dim`, `CNIF`: compiled into a side buffer and run, stays).

## Results that change

None for the reworks (rule 3; the check of the series). The fix of #1212 changes the executables of assignments whose left side has code (an array index): the right side's code comes before the address code, and a6 is loaded where the right side needs it; named in its commit, checked with `-DPREV_INTENDED`.

## Capabilities

### New Capabilities

(none; `skip_specs`)

### Modified Capabilities

(none)

## Impact

`asmlibs.c` (`countpees`), `eval.c` (the intrinsics, `libfunction`, `getap`, `maineval2`), `compile.c` (`dolibtoke`'s `intrinsic_code`), `tokens2.c` (`dolet2`), `tokens1.c` (`doselect`), `libcall.c` (`pushstart`, `pushdo`, `pushput`). After `expr-tree-4a-emit-after-build`. Overlaps: `ab3-mode-g5` edits `tokens2.c` (`doend3`, `stateret`, `funcret`) and `asmlibs.c`; `buffer-access-annotations` names `tokens1.c`; different functions, no order set (planning choice).
