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

## Findings (implementation)

- Task 1.1's forms: listed from the branches of `function`, `libfunction`, `dolibtoke`, `fetchlibps` and the intrinsics (`intrfind`, `intrstrvar`, `intrinsic_argpush`, `intrinsic_quick`, `intrinsic_code`) rather than with a coverage build, as in `expr-tree-3c-calls`. The new test `tests/blitz2/expr-libcalls.test.bas` (BASIC test model, without and with runtime error checking, its own prev pass `blitz2_expr_libcalls_prev`) compiles and runs each of them: libraries used first in another library function's parameter (`Hex$(Peek.w(...))`, `Sqr(Abs(...))`, `NumDays(Date$(...))`, the first case of the program), functions without parameters and with each of their forms (`Rnd`, `Mid$`, `Instr`), a size specifier (`Peek.b`, `.w`, `.l`, none), parameters of unknown type of each type (`Str$`, `Abs`, `Sgn`), a string result in a string expression and a number in a string function's parameter, calls at `regat` 5 and 6, and the intrinsics: `Int` and `Frac` with a Quick, a Long and a Float argument, as the whole right side (`Frac`: also before a `:`) and inside an expression, nested; `Len` and `Asc` with a plain string variable (also with spaces), an empty one, one without memory and an expression. Compiled and not run (behind `If never=1`): `PeekSound` in a string expression (#46: a3 saved with the movem), `GTSetAttrs` with one and two repeats (the repeats pushed: `preplibst`, `movenumrep`, `addq` and `fixlibst`), also at `regat` 2, and `Int`, `Frac` of a string (the library refuses it at run time).
- Decision 1 as built: `looktoke` (`asmlibs.c`) is the lookup alone ("Token Not Found" its only side effect), `findtoke` is `looktoke` and `uselib3` as before. `function` (`eval.c`) looks its token up with `looktoke`, makes the call node (`CLK_LIB`, or `CLK_OS` for an OS call, which `amigalib` then takes) and uses the library right after (`calluse`, the first part of the node's enter action), before it reads the function flags: the moment `findtoke` used it, so "Syntax Error" for a token that is no function and "Illegal Constant Expression" in a constant expression keep their order. Every other caller keeps `findtoke`: `handlelib` (statements), `tokejsr`, `fetchit` (`Used`, `Addr`), `getmaxlib` (`Maximum`), `tokecode` and `maketjsr` (the library routines' jsrs).
- The enter action is in three parts and the exit action in two, because reads of the source lie between them at the moments of before (`getchar3` can act on the source): the use (`calluse`) after the lookup; `libenter` (the string base, the registers below `regat`, a3 for #46) after the size specifier; `libregs` (`regat` kept and 0, the size specifier's type into d0) after the brackets were counted (`countpees`); the exit: `libexit` (the call: `libjsrcode`) after the parameters, `libresult` (the result into `D(regat)`, the movem back, a string's length) after the `)`. Then the node is the leaf of the call's value (`callcont`); `varcont`, which made a leaf of its own, has no caller left and is gone.
- `dolibtoke` (`compile.c`) is in parts that both ways call: `libform` (the form), `libparams` (`fetchlibps` or the char after a call without parameters), `libjsrcode` (`preplibst`, `movenumrep`, the jsr or `intrinsic_code`, `addq`/`fixlibst`). The statement path keeps `dolibtoke`; a library function calls them through `libcallnode` with the same save and restore of `numreps`, `userp` and `stackuse`.
- Decision 2 as built, narrower than its text: the node stores the form (`off`, its offset from the entry), not a type word per parameter. Each parameter is its expression's node, compiled and pushed as it is read (as the parameters of step 3c); the wanted type of a parameter is the form's type byte, and the type word of a parameter of unknown type is pushed by `maineval2` at that parameter's moment, from the type its expression got. A record per parameter would need storage per call beyond the node; no emitted byte depends on it here. For change 8 (typing before emission) the form's type bytes give the wanted types; the type words come from the parameters' subtrees once they are kept.
- The intrinsic state in the node: `prec` the entry of `intrinsics[]` (its index + 1, 0 none) and `CLF_INLINE` when the call was compiled inline (the node's type is then the entry's result type); the state machine (`intrcur`, `intrstate`, read by `getap`, `maineval2` and `libjsrcode`) stays as it was, saved and restored around a nested call by `libfunction`, until `expr-tree-4b-lookaheads`.
- The bound of the arena stays 161, `EXPRTREE_NODES` 168 (7728 bytes): a library call's node lives while its parameters are read, as a procedure call's, and counts among the at most 32 nested calls (`forthstack`). Measured (a throw-away counter build of `blitz2-ab3`, not committed): 16 nested `Abs` 34, 31: 64, 32: "Expression too Complex" at 65, 16 nested `Int` 34, 8 times `Str$(Len(` 34, 8 times `Abs(f{` 34, 10 times `-Abs(` and 21 times `-f{` before 32 times `-(` and `-b` 160 (and with `-f{` first), with 22 times `-f{` "Precedence Stack Overflow" at 161; the #1080 case (31 nested `Abs`) 65 -> 96, 16 `Right$` 22 -> 40; the cases of step 3c unchanged (10 parentheses 14, 30 and 31 arrays 32 and 33, 31 behind minus signs 64, 16, 30, 31 nested calls 34, 62, 64, 31 times `-f{` and 32 times `-(` 160, 32 times "Precedence Stack Overflow" at 161, 16 OS calls 36). The case of 31 times `-Abs(` with 32 times `-(` does not fit the 250 characters of a main-source line in AB3 mode (`docs/ab3/variant.md`), hence the mixed one.
- The stack (the runner of step 2, `blitz2-ab3` with a 200000-byte stack, compile only, the programs run under `reqtrap`; main = d2a26d1de): 10 parentheses 3919 -> 3743, the #1080 case 23579 -> 22183, `chr_mem.ab3` 3067 -> 2959, 16 `Right$` 10027 -> 9499, 30 nested arrays 13487 -> 12991, 31: 13887 -> 13375, 31 behind minus signs 17235 -> 16227, a path of 40 pointers 1635 -> 1611, 16 nested calls 7919 -> 7647, 30: 13519 -> 13023, 31: 13919 -> 13407, 31 times `-f{` and 32 times `-(` 28383 -> 26335 (with `-g{}` 28367 -> 26319), 16 OS calls 6391 -> 6119, 16 nested `Abs` 9007 -> 8543, 31: 16027 -> 15143, 16 nested `Int` 9007 -> 8543, 8 times `Str$(Len(` 8979 -> 8515, 8 times `Abs(f{` 8463 -> 8095, the mixed bound case 29063 -> 26895: no case deeper (`dolibtoke`'s frame is no longer part of a library call's recursion, the call's state is in the node); the programs' outputs are the same. Against main before step 2 (the #1080 case 22811) the figures stand.
- `blitz2` 115400 -> 115464 (task 2.1) -> 115712 bytes, `blitz2-ab3` 154288 -> 154352 -> 154600.
- The planning choices: the library use stays at its moment (the lookup), as planned; the side-effect table of `docs/blitz2/expression-tree.md`, "Step 3d", lists it under "At parse" for change 8. `ab3-mode-g5`: this change edits in `asmlibs.c` only `findtoke` (the new `looktoke` beside it) and its declaration in `asmlibs.h`, so the order stays open as planned.
