# Design

## Context

The series and its check: `expr-tree-1-typing/design.md`; the tree built before emission: `expr-tree-4a-emit-after-build/design.md`.

## Goals / Non-Goals

**Goals:** no source rescans and no code copies in expression compiling.

**Non-Goals:** the statements' own look-aheads outside expressions.

## Decisions

### 1. Parameter counts from the node

`countpees`'s callers get the count from the call node; where a statement (not an expression) calls it, it stays.

### 2. Intrinsics as node properties

The conditions the state machine collects are computed on the node at build time; the inline code is emitted at the place the state machine emits it today.

### 3. Assignments in emission order, not by copying

The left side of `dolet2` is built first (reading order: variables created as today), the right side next; emission is right side, then the address code, then the store: the bytes the copy produces today. Relocations of the address code are added at its emission, so the adjustment `pushput` makes goes away.

## Risks / Trade-offs

- [The copied address code today carries relocations whose order in the LIFO list differs from emission order] -> the check compares executables including their relocation hunks; task 1.1 has assignments with relocated left sides (arrays of a NewType in a global, string arrays).
- [`doselect`'s copy has its own protocol] -> cases in 1.1.

## Planning choices (mine, not the user's)

- `Point: ab3-mode-g5 and buffer-access-annotations edit tokens2.c / tokens1.c / asmlibs.c beside this change, no order set — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
