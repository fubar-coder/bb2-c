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

## Findings (implementation)

- Task 1.1's forms: listed from the branches of `afunction` and `amigalib` (without and with parameters, each result type under each kind of type so far, a string result and a call in a string expression: `makesbase`, a3 pushed and back by `deca3` or `geta3`, the registers below `regat` saved: `savem`, `chkstak` with runtime error checking; an OS call without brackets, with empty ones and with parameters, the parameter registers below `regat` of #344) rather than with a coverage build. The new test `tests/blitz2/expr-calls.test.bas` (BASIC test model, without and with runtime error checking, its own prev pass `blitz2_expr_calls_prev`) compiles and runs each of them, a call nested in its own parameter, a recursion, calls at `regat` 5 and 6 and OS calls twice in a row, after and before a library function and in a procedure's parameter. The a4/a5 parameters of #958 are compiled by `compiler-amigalib-a4a5` (an OS call in an expression in its child), the #344 registers also by `compiler-amigalib-param-regs`, both with their prev pass. AmiBlitz3's procedure forms in expressions (left-out optional parameters, parameters 7 and 8 on the stack, in a call at `regat` above 0, a string Function with a default in a string expression) are the new case `tests/ab3/cli/proc_expr.ab3` (also checked by AmiBlitz3's compiler, `ab3_cli_ref`).
- A procedure used before its definition is "Procedure not found": `findproc` finds only the procedures defined so far, there is no forward-reference path for a call in an expression (design, "Risks"). A test of the BASIC model cannot expect a compile error, so the case is named in the test's header only.
- The context's "`amigalib` loads the library base unless `lasta6` already holds it" does not hold: `amigalib` loads it for every call (`move.l off(a5),a6`) and does not read `lasta6`. Two OS calls in a row load it twice, as the test pins. It does not set `lasta6` either, which is an open original bug (below, `compiler-amigalib-lasta6`).
- Found while writing the test: an OS call in an expression after a Float operation (or a library function that takes a base in a6) leaves `lasta6` naming the FFP base while a6 holds the OS library's: the FFP calls after it (the conversion of the OS call's result, the operation) jump through the OS library's base (`Abs(-2)+FindTask_(0)`: a Privilege Violation; with mathieeesingbas.library, laid out as mathffp's, a wrong value). The original 1.95 does the same (`blitz2.s:10183-10258` has no `move #-1,lasta6`). An open original bug of class (a) (D-021: valid code, normal use; it crashes); `BUG(orig)` note in `eval.c` `amigalib`, bug-orig test `compiler-amigalib-lasta6` (REPRODUCED, the originals cell with 1.95 shows the same value). Not fixed here (the change changes no result).
