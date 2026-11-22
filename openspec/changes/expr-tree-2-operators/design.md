# Design

## Context

See `expr-tree-1-typing/design.md` ("Context", "The series"). `eval5` (`eval.c`): `pushprec(*op, prec)`, `regat++`, recursion for the right operand, `regat--`, `popprec`, conversion, `doop`, fold over `forthsp[-2..-1]`. `eval4` pushes a sentinel of precedence 1 per bracket and per `NOT`. #1080: 272 bytes of C stack per bracket level, about 490 per nested call; the compiler runs on a 48 KB stack of its own.

## Goals / Non-Goals

**Goals:** nodes and an arena that later changes can fill with element nodes; iterative emission along a chain; the limits as explicit, unchanged checks.

**Non-Goals:** deferred emission; dynamic memory.

## Decisions

### 1. Window, not whole tree

An operator node and its leaf operands are released when the operator's code is emitted (the moment `doop` runs today). The live part is the pending operators and their left operands: at most 64 operator and 32 element entries, the bounds of today's two stacks, so one static block covers it (size measured: task 1.3). Alternative: the whole expression from now on - needs dynamic blocks and deferred emission, which is change 8's subject; doing it here would move side effects before the elements are nodes.

### 2. The limits as counters at today's moments

`pushprec`'s overflow and `getelement2`'s `forthstack` overflow are counted as today (`forthstack` stays for the fold until change 8); `regat >= 7` stays in `eval5` before the right operand (#1185). A test source per limit pins message and line (task 1.1).

### 3. Iteration along the left spine

Today `eval5` loops for operators of equal or lower precedence and recurses for higher precedence and brackets; the node builder keeps exactly this shape (the loop becomes the iteration, the recursion stays where it is). A host test builds a chain of 100 000 equal-precedence operators in the arena code compiled for the host and checks that the walk's stack use does not grow with the length (the host build with a small thread stack, or the sanitizer's stack-use report).

## Risks / Trade-offs

- [The release point differs by one operator from today's emission point] -> bytes differ: the check catches it.
- [Frame size of `eval5` grows with the node bookkeeping] -> the #1080 depths measured again (task 2.3); the compiler's 48 KB stack has room (63 brackets measured 18683 bytes).

## Planning choices (mine, not the user's)

- `Point: window release keeps the static block sufficient; whole-tree arena deferred to change 8 — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
- `Point: stack measurement by a throw-away fill-pattern runner as in #1080 (report only, not a committed tool) — L2 Possible / I1 Low / E2 Reasoned → Monitor` (a permanent tool would be a C tool in `tools/`, rule 6, its own decision).
