# Proposal

## Why

Sixth change of the expression-tree series (#1180 step 3; the series: `expr-tree-1-typing/design.md`). Library calls are the element with the most forms: `function` -> `libfunction`, `dolibtoke` (`compile.c`), `fetchlibps` with repeats, unknown-type parameters with their type words, the intrinsics (`Int`, `Frac`, `Len`, `Asc`, a state machine threaded through `libfunction`, `getap`, `maineval2`, `dolibtoke`). The token lookup `findtoke` also makes the library's first use (`uselib3`: its place in the executable and its variable offset).

## What Changes

- Call nodes for library functions with their parameter subtrees and the form chosen (`fetchlibps`), emitted at today's moments.
- The lookup separated from the use: `findtoke`'s search and `uselib3` become two calls; the node's enter action makes the use at today's moment (before the parameters), so library order and offsets do not change.
- The intrinsic state stored in the node next to the state machine (the machine stays until `expr-tree-4b-lookaheads`).
- The side-effect table for these kinds.

## Not in scope

`countpees` and the intrinsics' text look-aheads (`expr-tree-4b-lookaheads`), runtime routine calls by operators (`tokejsr`, already emitter code), library calls as statements (`dolibtoke` from `compile.c` keeps its statement path).

## Results that change

None (rule 3); the check of the series, with `ab3_cli_prev_*` and the site examples (many library forms).

## Capabilities

### New Capabilities

(none; `skip_specs`)

### Modified Capabilities

(none)

## Impact

`eval.c` (`function`, `libfunction`, `getap`, the intrinsics), `compile.c` (`dolibtoke`, `intrinsic_code` call site), `asmlibs.c` (`findtoke`, `uselib3`, `fetchlibps`), `libcall.c`, `exprtree.c`, docs. After `expr-tree-3c-calls`. Overlap: `ab3-mode-g5` edits `asmlibs.c` (assembler tables) and `compiler-function-optimizer` edits `asmlibs.c` (label references): different functions; the optimizer not in parallel (eval.c), ab3-mode-g5 without an order (planning choice in design.md).
