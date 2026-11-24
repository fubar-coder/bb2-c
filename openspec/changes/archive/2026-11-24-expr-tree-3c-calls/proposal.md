# Proposal

## Why

Fifth change of the expression-tree series (#1180 step 3; the series: `expr-tree-1-typing/design.md`). Procedure calls (`afunction`, `fetchprocps`) and OS calls (`amigalib`) parse their parameter lists and emit the parameters and the call in one routine, calling `maineval` recursively per parameter. They become call nodes with parameter subtrees.

## What Changes

- Call nodes for procedure calls in expressions (`afunction`: the lookup, the parameters by `fetchprocps`, the register saves `savem`/`savereg`, the result in `D(regat)`, AB3's `ext_procstackbytes`/`ext_proctype`) and OS calls (`amigalib`: the library base in a6, `lasta6`, the register parameters).
- Emission at today's moments; the call's side effects that happen today before its parameters (the lookup, the first use of a library base) stay before them: the emit walk has an "enter" action before the children and an "exit" action after them.
- The side-effect table for these kinds.

## Not in scope

Library calls (`expr-tree-3d-library-calls`), procedure calls as statements (`handleproc`, not an expression), the function optimizer's call edges (`compiler-function-optimizer`).

## Results that change

None (rule 3); the check of the series.

## Capabilities

### New Capabilities

(none; `skip_specs`)

### Modified Capabilities

(none)

## Impact

`eval.c` (`afunction`, `amigalib`, `savem`, `savereg`), `asmlibs.c` (`fetchprocps` only through its caller), `ab3proc.c` hooks' call sites, `exprtree.c`, docs. After `expr-tree-3b-arrays`. Overlap: `compiler-function-optimizer` edits `afunction` and the label references in `eval.c`: not in parallel (recorded: eval.c changes one after another); the tree goes first by the approved plan ("Go for steps 1-7"), the optimizer's place after it is not set by this change.
