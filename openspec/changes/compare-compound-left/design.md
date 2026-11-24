# Design

## Context

After `expr-tree-4a-emit-after-build` the comparison node has both subtrees before any code; the left subtree is typed by flow under the wanted type, and #1177's rule exists as a node rule for a one-element left operand. The series: `openspec/changes/archive/2026-11-24-expr-tree-1-typing/design.md`.

## Goals / Non-Goals

**Goals:** the left subtree of a comparison typed without the wanted type; exact compare for its type and the right operand's.

**Non-Goals:** arithmetic typing (step 5).

## Decisions

### 1. The comparison node hands no wanted type down

At build time the left subtree of a comparison starts with type so far 0 (as the right subtree, #1177), and its own operands decide its type. Because the tree is built before emission, this is known before the left subtree's code exists. The one-element node rule of change 8 becomes the special case of this.

### 2. Scope by the answer to open question 1

(a) for all compound left operands; (b) only where a Quick or Float is inside or a Long constant meets a Float context. (a) is one rule, symmetric with the right operand; (b) changes fewer executables. Task 1.1 counts both on the CD.

## Risks / Trade-offs

- [More executables change than for #1177 (173 of 496 there)] -> counted before the fix (1.1), shown with question 1.
- [A whole-number compound comparison changes its value] -> spec requirement 2 and the corpus outputs; a Word sum in its own type is Long (Word+Word), as under a Long context today.

## Rating of the recommendation (mine, for the user's decision)

`Point: scope (a), every compound left operand typed by its own operands — L2 Possible / I3 Observable / E2 Reasoned → Decide` (also a recorded answer for another case bears: #1177's right operand, "Also so wie der Plan es vorgesehen hat").
