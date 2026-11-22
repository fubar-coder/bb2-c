# Design

## Context

`afunction` looks the procedure up, checks and compiles each parameter with `maineval` (pushing it), emits the `jsr` and fetches the result; `savem`/`savereg` choose the `movem` mask from `regat` and `fuckpos`. `amigalib` loads the library base unless `lasta6` already holds it. The series: `expr-tree-1-typing/design.md`.

## Goals / Non-Goals

**Goals:** call nodes with enter/exit actions; the parameter subtrees built by the evaluator's builder.

**Non-Goals:** changing parameter counting (`countpees`, change 9), register saving (step 6, not planned).

## Decisions

### 1. Enter and exit actions

A node's emit has two parts: "enter" (what today happens before the children are compiled: the lookup's effects, a register save) and "exit" (the call, the result move, the restore). With emission at today's moments this reproduces the order; change 8 relies on it when it emits after the build.

### 2. Parameters keep their push protocol

Each parameter subtree is emitted with today's push (`maineval2`'s push flag, `pushflag`, `stackuse`). The node only records the parameter's wanted type and push mode.

## Risks / Trade-offs

- [`lasta6` and the `movem` mask depend on state at the moment of emission] -> unchanged moments; the check.
- [A procedure defined after its use] -> the lookup's forward-reference path is a compared case (task 1.1).

## Planning choices (mine, not the user's)

- `Point: enter/exit actions per node instead of "all library use at emit time" (the analysis's rule would move a call's library before or after its parameters' libraries) — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
