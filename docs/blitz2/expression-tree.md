# The expression evaluator restructured: from blitz2.s to the C functions

`blitz2` compiles an expression the way the original does: in one pass,
emitting each element while it reads it (`docs/blitz2/expression-types.md`).
Issue #1180 restructures this evaluator in steps (the OpenSpec changes
`expr-tree-*`): first the type decisions are collected in one place, later
the expression becomes a tree that is built and then emitted. The
compiled programs stay byte-identical until the step that changes a type
decision on purpose.

A restructured routine no longer matches one label of `blitz2.s`. Such a
C function begins with `/* no asm counterpart; replaces blitz2.s:NNN LABEL
... */`, naming the original lines it takes over, and this page maps
every original routine or part of one that moved to where it is now. A
routine that keeps its algorithm keeps its `/* blitz2.s:LINE label */`
head.

## Step 1: the type decisions (`expr-tree-1-typing`)

The decisions are in `programs/blitz2/src/exprtype.c` (pure C, pinned by
the host test `blitz2_exprtype` and its table
`programs/blitz2/tests/host/exprtype.tsv`); the routines of the evaluator
emit what the functions return. The code they emit is the same as
before.

| Original (`blitz2.s`) | What it decided | Now |
|---|---|---|
| 8989-8993 `eval5` `.higher`: `move d2,-(a7)`, `bsr eval5` | the right operand starts with the type so far of the left one | `exprtype_leftop` (`compare`: a comparison starts it at 0, #1177; a raw left Quick is a Quick, #1178), called by `exprtree_pre` |
| 8995-8998 `eval5`: `move (a7)+,d3`, `beq .skip`, `bsr convtypef` | the left operand converted to the type so far after the right one | `exprtype_binop` (`lconv`), emitted by `operate` (`eval.c`) |
| 8999-9000 `eval5` `.skip`: `bsr popprec`, `bsr doop` | the handler of that type | `exprtype_binop` (`handler`, `result`), emitted by `operate` |
| 9142 `doop` | "Illegal Operator for Type" for type 0 or a type without a handler | `exprtype_optype` (0: none); `doop` raises the error and calls the handler |
| 14704 `operators` and the handlers' result types: 9322 `cpoke`, 9334 `doplusb` ("now a word"), 9337 `doplusw` ("now a long"), 9402 `dotimesb`, 9406 `dotimesw`, 9428 `dopow`, 9459 `domodq`, 9463 `domodf`, 9547 `shpoke`, 9572 `bittst`, 9586-9596 `bitset`, `bitclr`, `bitchg` | the type an operation gives | `exprtype_optype`'s table (the handlers still emit their code, and their own conversions: `shpoke`, `dothebit`, `dopow`, `dodivb`) |
| 10479-10487 `varcont`: `ext.w Dn` | a Byte value is held as a Word | `exprtype_element` (`ext`), emitted by `varcont` |
| 10488-10498 `varcont` `.notabyte`, `.nob`, `.conv2` | the value converted up to the type so far, or the type so far raised to the value's type; a Quick under a Long cut (the port: held raw, #1178) | `exprtype_element` (`conv`, `result`, `raw`), emitted by `varcont` |
| 10913-11013 `consttype2` (`.something`'s switch, `.byte`, `.word`, `.long`, `.quick`, `.qint`, `.float`, `numdofloat`) | the constant's type from the type so far and its size | `exprtype_element` with the constant's facts (`exprtype_constfacts`); `consttype2` finds the facts and makes the value (`quickconst`: `.quick2`) |

The port's own decisions (no asm counterpart) moved as well:

| Port routine (before) | Rule | Now |
|---|---|---|
| `rawcutop` (#1178) | a raw Quick consumed by a shift, a logic or bit operator or Mod is rounded down at once | `exprtype_cutsraw` |
| `eval5` (#1177, #1178) | a comparison's raw left operand is a Quick; a one-element left operand may get its own type back | `exprtype_leftop` |
| `cmpownleft`'s condition (#1177) | which conversion of `varcont` is dropped | `exprtype_ownleft` |
| `eval5` (#978) | which constant of an equality comparison with a byte value is sign-extended | `exprtype_binop` (`byteext`) |
| `eval5` (#1182) | a right operand lowered by a comparison converted up to the left one's type | `exprtype_binop` (`rup`, `cmpres`) |
| `eval5`, `compareop`, `cmplq` (#1177) | a Long against a Quick exactly, otherwise the lower operand up | `exprtype_binop` (`path` compare or cmplq) |
| `eval5`, `mixedop` (#1178) | a raw Quick under a Long, or a Quick made Long under `*` and `/` | `exprtype_binop` (`path` mixed, `lq`, `rq`, the conversions) |
| `eval5` (#1179) | `*` and `/` of two Word values in a later index inline | `exprtype_binop` (`wordop`) |

What stays where it was: the patch mechanisms that act on the decisions
(the constant fold `constfold`, the #978 patch `byteconstext`,
`cmpownleft`'s rewind, `fraclitfloat`), the end of an expression
(`maineval2`), the unary operators, and the code of each conversion and
handler.

## Step 2: the operators as nodes (`expr-tree-2-operators`)

Each binary operator, each bracket, each `NOT`, each run of minus signs
and each expression (the one of a statement, a function argument, an
index) is a node (`programs/blitz2/src/exprtree.c`, `struct exprnode`);
each operand is an opaque leaf that records where its code begins and
ends. The nodes live in a static arena of `EXPRTREE_NODES` (136) that
`prepstack` resets for every statement. A node is released when its
operator has been emitted, at the moment the original emitted it, so the
arena holds only the window of the pending operators and their left
operands: at most the 64 counted nodes, the 65th before its count, and
one leaf or one run of minus signs per level (129; 63 times `-(` keep 128
live). The elements are still emitted while they are read, the constant
fold still works on `forthstack`. The code the compiler emits is the
same as before.

The walk `exprtree_climb` is `eval5`'s control: an operand, then each
operator that binds tighter than the pending one with its right operand
(one level deeper, as the original recursed) and its emission; along a
chain of one precedence it loops, so a chain of any length needs no
stack. What the original did in between is in the walk's hooks in
`eval.c`, called at the same moments. The limits stay where they were:
"Precedence Stack Overflow" at the 65th counted node (the expression,
each bracket, each `NOT`, each pending operator), "Expression too
Complex" at the 33rd pending element (`forthstack`) and for an operand
beyond d7 (#1185). The host test `blitz2_exprtree` runs the walk with an
evaluator of its own; `tests/blitz2/expr-limits.test.bas` pins the
limits.

| Original (`blitz2.s`) | What it did | Now |
|---|---|---|
| 9171 `precsp`, 9173 `precstack` (6 bytes x 64) | the pending operators and the sentinels of brackets and `NOT` | the counted nodes of the arena (`exprtree.c`: `pend`, `npend`, `struct exprnode.down`) |
| 9153 `pushprec` | push an operator and its precedence | `exprtree_push` (the limit checked before, as the port did) |
| 9161 `popprec` | pull it after the right operand | `exprtree_release` of the operator's node after its emission (`exprtree_climb`); the original's check there cannot be reached (the push checks) |
| 8972-8980 `eval4` | the sentinel of precedence 1, `eval5`, `subq.l #6,precsp` | `eval4` makes the expression's node (`EXN_EXPR`, `EXN_BRACKET`, `EXN_NOT`) and calls the walk; its caller releases the node (`eval4end`): `eval3`, `bracket`, and `notit` after its `not` |
| 8983-8987 `eval5`, `.more` | the operand (`bsr eval2`) and the test of the next operator against the pending one | `exprtree_climb`; the operand: `exprtree_operand` (`eval2` gives the operator's index), the leaf and the raw Quick cut of #1178: `exprtree_left`, the end of a level: `exprtree_done` |
| 8989-8992 `eval5` `.higher` | `bsr pushprec`, the left type saved, `addq #1,regat`, `bsr eval5` for the right operand | `exprtree_climb`; before the count `exprtree_pre` (#1185, #1177, #978's facts), after it `exprtree_enter` |
| 8993-9083 `eval5` after the right operand | `subq #1,regat`, the conversion, `popprec`, `doop`, the constant fold | `exprtree_emit` (`operate`, `constfold`) |
| 9667 `negate` and its recursion through `getelement2` | one `neg` per minus sign, innermost first | `negate`: a run of minus signs is one node (`EXN_NEG`, the count) and a loop; then one `neg` (or `SPNeg`) per sign |
| 9718 `getelement2` | `bsr getchar3`, then the element | `getelement2` reads the char, `element` the element (so that `negate` can hand it the char after the run) |
