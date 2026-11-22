# Proposal

## Why

Third change of the expression-tree series (#1180 step 3, first element kinds; the series: `expr-tree-1-typing/design.md`). Step 5 needs the type of an element before its code is emitted, so every element kind gets a parse part that returns a leaf node with its own type and facts, and an emit part. This change does it for the simple elements, which have no subexpressions of their own.

## What Changes

- Leaf nodes and the parse/emit split for: decimal, `$` and `%` literals and `#constants` (`consttype2`, AB3's `ext_constant`), scalar variables and NewType fields without an index (`variable` -> `fetchvar`, `variable2`, `varcont`), `&var`, `?label` (`makealab`), `SizeOf`, `Pi`, `On`/`Off`, `Used`, `Maximum`, `Addr`, `ErrNum` (`doused`, `domaximum`, `doaddr`, `doern`), AB3's `ext_element` (character literals).
- Emission stays at today's moment: the emit part runs right after the parse part (window, as `expr-tree-2-operators`).
- Each element's side effects get their time in a table (`docs/blitz2/expression-tree.md`, new): at parse (a variable is created and gets its offset at first sight, in source order) or at emit (relocations, label references, literals, library use by a conversion); and before or after the element's children where it has any. Change 8 relies on this table.
- The facts `varcont` and `consttype2` leave for later code (`bytevalend`, `constat`/`constend`, `conv` of #1177, the raw-Quick flag of #1178, `fraclit` of #1186) are also stored in the leaf; the old globals stay as long as their readers do (change 8 removes them).

## Not in scope

Arrays and pointers (`expr-tree-3b-arrays`), calls (`3c`, `3d`), strings (`3e`), deferred emission (`4a`); any type decision.

## Results that change

None (rule 3); the check of the series.

## Capabilities

### New Capabilities

(none; `skip_specs`)

### Modified Capabilities

(none)

## Impact

`eval.c` (`getelement2`, `variable`, `varcont`, `makealab`, `doused`, `domaximum`, `doaddr`, `doern`), `libcall.c` (`consttype2` and the number fetches), `parse.c` (`fetchvar` only through its callers), `ext_bb2.c`/`ab3chr.c`/`ab3syn.c` (the hooks' call sites), `exprtree.c`, `docs/blitz2/expression-tree.md` (new). After `expr-tree-2-operators`.
