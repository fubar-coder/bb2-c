# Proposal

## Why

Seventh change of the expression-tree series (#1180 step 3; the series: `expr-tree-1-typing/design.md`). String elements and string operations follow a protocol that depends on the order of emission: the work area (`sbasegot`/`sbgot`, `makesbase`), the lengths on the stack (`stackuse`, `sbtolong`), `addstrings`, the comparisons through `strcomp`, literals written to the data area (`litstring`).

## What Changes

- Leaf nodes for string literals, string variables and string function values; operator nodes of string type for `+` and the six comparisons; all emitted at today's moments and in sequence (the string protocol stays strictly ordered).
- The side-effect table for these kinds (literal area, work area, stack use).

## Not in scope

String functions' library forms (done in `expr-tree-3d-library-calls`), the assignment of strings (`dolet2`, change 9).

## Results that change

None (rule 3); the check of the series.

## Capabilities

### New Capabilities

(none; `skip_specs`)

### Modified Capabilities

(none)

## Impact

`eval.c` (`litstring`, `addstrings`, the string comparisons, `makesbase`, `sbtolong`), AB3's `ext_strescape`/`ext_strpointer` call sites, `exprtree.c`, docs. After `expr-tree-3d-library-calls`.
