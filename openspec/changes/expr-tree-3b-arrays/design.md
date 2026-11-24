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

## Findings (implementation)

- Task 1.1's coverage: `calcvar`'s branches (45 of them: the array, its List and indexed forms, the size and bound checks, the element size of a pointer array, a local array's dimensions, a later index, the pointer of a pointer array, the simple, pointer and NewType variable, the field, the field array `[ ]` with and without its check, the pointer field followed or as a value, at offset 0 or above, a BCPL pointer, the offset of the end: 0, 1 to 8, 9 and above; and each error) were marked with an empty function each in a throw-away build of `amiga-cov` (not committed), and a run of the compared sources (`corpus*`, `blitz2_*`, `regression_*`, `ab3_cli*`, `site_examples*`, `compiler*`, `bug_orig_compiler*`: 676 tests) showed which ones they reach. Without the new test no compared source compiled the element size of a pointer array (`Dim *pp.rec(3)`), its pointer followed or taken as a value, a pointer field array (`*ps.in[3]`, only an AB3 test), a pointer field at offset 0 followed (`n1\nxt\v`), a BCPL pointer followed (`@bp.in`), a field array at offset 0 (only the CD with AB3) and the `[0]` tail without a bound check. The new test `tests/blitz2/expr-arrays.test.bas` (BASIC test model, without and with runtime error checking, its own prev pass `blitz2_expr_arrays_prev`) compiles and runs every form of the task and reaches every branch but the errors: "Array is not a List" for `a()` of a normal array, the syntax errors of `,`, `)` and `]`, "Array not yet Dim'd" for `(` after a variable that is no array (and for an array used before its `Dim`: only a CD source), "Mismatched Types" for a field of a simple type and `[` after a field that is no array, a field without a name and "Offset not Found". They are the parse part's, raised where they were.
- An `@` variable cannot be read in an expression (`element` has no `@`: "Syntax Error"), only an `@` field; a test of the BASIC model cannot `Dim` outside its blocks, so the Shared array is made in the batch init and the function that shares it follows it.
