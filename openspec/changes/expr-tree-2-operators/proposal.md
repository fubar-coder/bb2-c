# Proposal

## Why

Second change of the expression-tree series (#1180 step 2; the series and its check: `expr-tree-1-typing/design.md`, "The series", "The check of every byte-identical step"; after archive in `openspec/changes/archive/`). `eval5` keeps its pending operators on `precstack[64]` and its pending elements on `forthstack[32]` and recurses once per precedence level and bracket. This change gives operators, brackets, unary minus and `NOT` a node each, built in an arena per statement, and walks that do not recurse along a chain of one precedence, while the elements are still emitted as they are read. It proves the node layout, the arena, the limits and the walks without moving any side effect. On its own it buys nothing for programs (the analysis's "scaffolding").

## What Changes

- **Nodes for operators, brackets, minus, `NOT`** (new `programs/blitz2/src/exprtree.c`/`.h`): kind, operator, type so far before and after, register, and the facts `exprtype_binop` takes; an element is an opaque leaf (its code is emitted when read, as today, and the leaf records where it begins and ends).
- **Arena per statement**: one static block (size by measurement, about 4 KB, task 1.3), reset where `prepstack` resets the two stacks (`setsvars`). In this change the nodes of an operator are released when the operator is emitted (today's moment), so the live nodes are bounded by today's limits and the static block suffices; no allocation, no new error.
- **The limits stay** with today's messages and at today's moment: "Expression too Complex" at 32 pending elements and at `regat >= 7` (#1185), "Precedence Stack Overflow" at 64 pending operators; they become explicit counters.
- **The walk** that emits an operator after its right operand iterates along the left spine; the C recursion stays only where `eval5` recurses today (a higher precedence, a bracket), so the stack depths measured in #1080 stay valid or shrink.

## Not in scope

- Element nodes (changes `expr-tree-3a-leaves` to `expr-tree-3e-strings`), emission after the build (`expr-tree-4a-emit-after-build`), the constant fold's mechanism (it keeps `forthstack` in this change), any type decision (step 1's function is used as is).
- Removing or raising the limits (a named change, not planned).

## Results that change

None (rule 3); the check of the series. `blitz2` itself changes (rework).

## Capabilities

### New Capabilities

(none; `skip_specs`)

### Modified Capabilities

(none)

## Impact

- `programs/blitz2/src/eval.c` (`eval4`, `eval5`, `eval2`, `negate`, `notit`, `bracket`, `pushprec`/`popprec`), new `exprtree.c`/`.h`, `compile.c` (`prepstack`), `CMakeLists.txt`.
- Host tests of the arena and the walk (`programs/blitz2/tests/host`), a compiler test of the limits (`tests/blitz2`).
- After `expr-tree-1-typing`.
