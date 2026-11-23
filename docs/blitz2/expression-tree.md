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
| 8989-8993 `eval5` `.higher`: `move d2,-(a7)`, `bsr eval5` | the right operand starts with the type so far of the left one | `exprtype_leftop` (`compare`: a comparison starts it at 0, #1177; a raw left Quick is a Quick, #1178), called by `eval5` |
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
