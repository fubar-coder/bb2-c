# Design

## Context

`calcvar` (`parse.c`) is called from `variable` and from statements (the left side of `Let`, `Dim`, `For`, `Read` ...). It fetches the variable (`fetchvar` creates it at first sight), walks the NewType path, and for each index calls `arreval` (first index) or `arrevalchk` (later ones) which compile into `D(regat)`. The series: `expr-tree-1-typing/design.md`.

## Goals / Non-Goals

**Goals:** one variable node for every form `calcvar` accepts; the statements keep calling one entry with the same result.

**Non-Goals:** changing the statements' use of `calcvar`; the assignment's address copy (change 9).

## Decisions

### 1. One entry, two parts

The statements call `calcvar` as today; inside, "parse node, emit node". The expression path (`variable`) uses the two parts directly. This keeps the 50 `maineval` call sites and the statement callers untouched.

### 2. Coverage before the split

The forms are listed from `calcvar`'s branches; each form that no compared source compiles gets a compile-only case first (task 1.1), so that the check sees every branch. The coverage build (`testing/coverage`) tells which branches the compared sources reach.

## Risks / Trade-offs

- [A form reached by no compared source changes its bytes] -> task 1.1 closes the coverage gap before the split; a remaining unreached branch is named in the report.
- [Runtime checks move] -> the runerr passes compare the executables with checks on.

## Planning choices (mine, not the user's)

- `Point: ab3-mode-g5 and this change both edit parse.c in different functions, no order set between them — L2 Possible / I2 Medium / E2 Reasoned → Plan` (a merge conflict is resolved in the later branch).
