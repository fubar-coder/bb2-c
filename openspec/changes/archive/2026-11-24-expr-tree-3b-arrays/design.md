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
- The variable node and the parse part (decision 1): `varparse` (`parse.c`) is `calcvar`'s parse part; `calcvar` stays the statements' entry with its interface (it reads the node and releases it), `variable` and `ampersand2` call `varparse` and make the node their leaf (`varleaf`), so the 50 `maineval` call sites and the statements' callers are unchanged. A step of the path is a node of its own (`EXN_STEP`, its kind `VST_*` in `op`: the List's current item, each index, the end of the indexes, a pointer array's pointer, a NewType variable's base, the three parts of a field array `[ ]`, a pointer followed, the end of the path); its emit part `stepemit` writes the code from the step and the emitter's state only. Each step is made when its part has been read (after its index: the index is compiled into `D(regat)` while it is read, as every expression; its code is the step's `aux`..`start`), emitted at once and released: the window, as the leaves of step 3a. The variable node (`EXN_VAR`) is made at the end of the path. So the nodes do not hold the whole path at once: a variable node with its steps as children needs the emission after the build (step 4a), which would also make the arena hold them (and with them the bound grow). `addd0a0`'s clobbered d3, which `calcvar` returns as the offset, is the pure `addd0a0d3`, so the parse part does not read it back from the emission.
- The order of the code and of the errors is the original's: the errors that stand between two parts of the code are read between two steps (`,` after an index's check, "Type not yet defined" between the bound check and the scaling of a field array, which are two steps for that). The restores of `lastoffset` and `varmode` after a field array's index come before the `move.l (a7)+,a2` now (the asm restored them after it); the instruction does not read them. `varmode` at a step is the variable's at every step (the parse part restores it after each index as before); the step records it (`ELF_LOCAL`) and `stepemit` sets it from there.
- The bound of the arena stays 129 nodes (EXPRTREE_NODES 136, 6256 bytes): a step and the variable node take the place of an element's leaf (made after the index, released before the next node). Measured high water (throw-away counter in the `amiga-cov` build of `blitz2-ab3`): 10 parentheses 14, 63 parentheses 65, 16 `Right$` 22, the #1080 case 65, `chr_mem.ab3` 7, 63 times `-(` with `-b` 129, 31 arrays nested in their indexes 33, the same behind minus signs 64, a two-dimensional array nested 31 times 33, a path of 40 pointers 2, 62 times `-(` with `-a(-c)` 129 and with `-a(-b)\p\w[-b]` 129. The host test `blitz2_exprtree` pins the arrays nested behind minus signs (63: 129, 64: "Precedence Stack Overflow"). An array nested 32 times is "Expression too Complex" on main and here (the 33rd pending element of `forthstack`).
- The stack (the runner of step 2, `blitz2-ab3` with a 200000-byte stack; main = 6cc9b6f42): 10 parentheses 4007 -> 4007, 63 parentheses 16899 -> 16899, 16 `Right$` 10191 -> 10179, the #1080 case 24091 -> 24091, `chr_mem.ab3` 3111 -> 3099, 30 nested arrays 13975 -> 13735, 31: 14391 -> 14143, 31 behind minus signs 17987 -> 17739, a two-dimensional array nested 31 times 14407 -> 14159, a path of 40 pointers 1643 -> 1643: no case deeper (the frame of `varparse` holds less than `calcvar`'s did); against main before step 2 the figures of step 3a stand (63 parentheses +8.2 %).
- `blitz2` 114376 -> 115036 bytes, `blitz2-ab3` 153240 -> 153900.
- The planning choice (`ab3-mode-g5` and this change in `parse.c`, no order): no conflict met in this branch (main at 6cc9b6f42); `calcvar`'s code moved to `varparse` and `stepemit`, so a later change of `ab3-mode-g5` near it merges onto these.
