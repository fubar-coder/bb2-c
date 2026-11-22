# Design

## Context

`findtoke` (`asmlibs.c`) returns the token's library entry and calls `uselib3` on it, which copies the library into the library area at first use and gives it its variable offset; `tokejsr` does the same for runtime routines. The intrinsics decide inline code after the argument was compiled ("the argument turned out to be a Quick"). The series: `expr-tree-1-typing/design.md`; enter/exit actions: `expr-tree-3c-calls/design.md`.

## Goals / Non-Goals

**Goals:** library call nodes; a lookup that has no side effect, so that change 8 can type a call before it is emitted.

**Non-Goals:** removing the intrinsic state machine (change 9).

## Decisions

### 1. `findtoke` split

A lookup function without `uselib3`, and `uselib3` called by the node's enter action at the moment today's `findtoke` calls it. Every other caller of `findtoke` (statements, the assembler) keeps the combined call.

### 2. Forms with repeats and unknown types

The node stores the chosen form and, per parameter, the type word to emit; `fetchlibps`'s choice is made where it is made today.

## Risks / Trade-offs

- [A library first used inside a parameter of another library's function] -> today the outer library is used first (lookup before parameters); the enter action keeps that. Test case in 1.1.
- [Many forms, few compared] -> coverage first (task 1.1).

## Planning choices (mine, not the user's)

- `Point: ab3-mode-g5 and this change both edit asmlibs.c in different functions, no order set — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
