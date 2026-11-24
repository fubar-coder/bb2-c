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

## Step 3a: the simple elements as leaves (`expr-tree-3a-leaves`)

Each simple element, one without a subexpression of its own, has a parse
part and an emit part. The parse part reads the element and makes its
leaf (`struct exprnode` of kind `EXN_LEAF`, `op` its element kind
`ELK_*`, `exprtree.h`): the type so far it was read under (`tbefore`),
its own type (`own`), the type so far after it (`tafter`), its facts
(`lflags`, `ELF_*`) and what its code needs (`value`, `aux`, `vtype`,
`off`). The emit part writes the code from the leaf and the emitter's
state (`regat`, `lasta6`, the fold stack, the string base) only, and
records where it is (`start`..`end`). The emit part runs right after the
parse part and the leaf is released after it, as the operators of
step 2; the char after the element is read after its code, as before. So
the code and its moment are the same as before; step 4a
(`expr-tree-4a-emit-after-build`) keeps the leaves and emits them later.
The facts later code reads stay in their globals as well (`bytevalend`,
`conv`, `rawq`, `constat`/`constend`, `litat`; change 8 removes them).
An element's leaf lives only while it is emitted, so the bound of the
arena stays 129 nodes: behind the run of minus signs of the innermost
level it is the 129th (63 times `-(` and `-b`); a node is 46 bytes on
the 68000 (the block of 136: 6256 bytes).

| Original (`blitz2.s`) | What it did | Now |
|---|---|---|
| 9700 `fetchpi` | `move.l #Pi,Dn`, Float whatever the type so far | `fetchpi` makes the leaf (`ELK_PI`), `piemit` its code |
| 9801 `fetchsize`, 9839 `fetchon`, 9843 `fetchoff`, 9866 `fetchhex`, 9883 `fetchbin`, 9896 `fetchconst`, 10042 `domaximum`, 10449 `fetchqasm`, 10460 `fetchasm`, 10709 `fetchfrac`, 10712 `fetchnum`, 10737 `fetchpoint2`, 10902 `fetchnum2`; AmiBlitz3's character literal (`ab3chr.c`, `ext_element`) | read the number, then `consttype2` | unchanged: the parse part of a constant (`ELK_CONST`) |
| 10856 `fetchnum3` | the value of Data, or the code of the constant | `fetchnum3` (`libcall.c`) ends the parse part: the leaf with the value, `consttype2`'s type and its facts |
| 10860-10888 `fetchnum3` `.doit`, `.word`, `.long` | `move.w`/`move.l #x,Dn` | `constemit` (and the facts of #978 and #1186 for the code after it) |
| 9941 `doern` | `tokejsr ern` | `doern` (`ELK_ERRNUM`): the leaf and its code in one |
| 9989 `doused` up to `bsr fetchit` | the object's library | `doused` (`ELK_USED`, the library in `value`) |
| 9993-10037 `doused` after `fetchit` | `usedcode` or `usedcode2` patched, d0 saved when `regat` > 0 | `usedemit` |
| 10053-10067 `doaddr` up to `bsr reget` | the token, the index (`getmaxel`), the brackets | `doaddr` (`ELK_ADDR`, the maximum entry in `value`) |
| 10069-10083 `doaddr`'s code | `addrcode` patched, `pokecode` | `addremit` |
| 9910-9913 `ampersand2` up to `bsr calcvar` | the variable and its offset | `ampersand2` (`ELK_ADDROF`) |
| 9915-9939 `ampersand2` after `calcvar` | the address into Dn | `addrofemit` (with `ext_evalvalue`, called there as before) |
| 10352 `makealab`, 10353 `makealab2` | the label and a reference to it if it is not defined yet | `makealab2` is `labfind` and `labref` |
| 10353-10364, 10382-10389 `makealab2` without `.makeit` | find or make the label; "Label has been used as a Constant", "Label reference out of context" | `labfind` |
| 10368-10380 `makealab2` `.makeit` | the outstanding reference at `pc` + 2 | `labref` |
| 10406-10407 `qmark` | `?` and the label | `qmark` (`ELK_LABEL`, the label in `value`, `ELF_REF` when it needs a reference) |
| 10409-10418 `qmark`'s code | `move.l #label,Dn` and its relocation | `labelemit` (the reference first, as `makealab2` made it) |
| 10471-10477 `variable` up to `bsr calcvar` | the variable (made at first sight) and its offset or path | `variable` (`ELK_VAR`; a name with `{` is `afunction`, step 3c) |
| 10478 `bsr variable2`, 10641-10682 `variable2` | the value into Dn; the type in d2 (`moveq #3`, `move.b #3`, `and #255`) | `variable2` from the leaf; the type: `vartypeof` at parse |
| 10479-10500 `varcont` | `ext.w` of a Byte, the conversion up to the type so far | the decisions `contleaf` (`exprtype_element`) at parse, the code `contemit`; `varcont` (both, for the calls of steps 3c and 3d) |

What each element changes besides its code, and when (change 8 relies on
this table: what is at parse happens while the expression is read, in
source order; what is at emit happens when its code is written):

| Kind | Elements | At parse | At emit | Children | Errors |
|---|---|---|---|---|---|
| `ELK_CONST` | decimal, `$`, `%` literals, `#constants`, `SizeOf`, `On`, `Off`, `Maximum`; in a constant expression a label's value and `"abcd"`; AB3's `@"..."` | `#constant`: AB3's `ext_constant`, the labels (`#__debug`); `Maximum`: the library used (`findtoke`, step 3d); the fold fact (`ELF_FOLD`: `forthstack`'s `constgot` is 0; not for `Maximum` and `@"..."`, which `element` reaches after its test); `asmtype` toggled for a label in a constant expression | `constat`/`constend`/`constval` (#978, #1179), `litat`..`litend` (#1186); `wordval` of the fold stack (`element`, after the code) | none | parse: "Syntax Error", "Numeric Over Flow", "Constant Not Found", "Type Not Found", "Offset not Found", "Type not yet defined", "Illegal Token" (an object without maximum), "Type Mismatch" (`consttype2`), "Can't Convert Types" (`@"..."` under a Quick); emit: "Type Mismatch" for a type without a move (not reached after `consttype2`) |
| `ELK_PI` | `Pi` | none | none | none | none |
| `ELK_VAR` | a variable, a NewType field, a pointer, a string variable | the variable made with its offset at first sight (`fetchvar`, in source order; a local one: `ELF_LOCAL`); an array index, a pointer or a path emit their address code in `calcvar` (step 3b) | `varmode` from the leaf; AB3's `ext_evalvalue` after the value's code (as before: in reading order with emit right after parse; change 8 must keep that order); a string's base and `copstring` (`stvar`, step 3e); a conversion's library use (`convtype`: `qtof2` -> `tokejsr`); `wordval`, `bytevalend` (#978), `conv` (#1177), `rawq` (#1178) | `calcvar`'s code before the leaf (step 3b) | parse: `fetchvar`'s and `calcvar`'s ("Mismatched Types", "Array not found" ...) |
| `ELK_ADDROF` | `&var` | as `ELK_VAR` | `varmode`; AB3's `ext_evalvalue` before the code | as `ELK_VAR` | as `ELK_VAR` |
| `ELK_LABEL` | `?label` | the label made at first sight (`labfind`: in the label list from then on) | the reference to a label not defined yet (`labref`: its address `pc` + 2, the statement's line and procedure), the relocation (`addoff`) | none | parse: "Syntax Error", "Label has been used as a Constant", "Label reference out of context"; emit: "Out of Memory" (the reference) |
| `ELK_USED` | `Used Object` | the library used (`findtoke`, step 3d) | `usedcode` patched in place | none | parse: "Syntax Error", "Illegal Token" |
| `ELK_ADDR` | `Addr Object(n)` | the library used (`findtoke`) | `addrcode` patched in place | the index: its code (with the maximum check of runtime error checking and the shift) before the leaf (`getmaxel`) | parse: "Syntax Error", "Illegal Token", the index's |
| `ELK_ERRNUM` | `ErrNum` ("Ern": no name in the token list, no source text reaches it) | none | the library routine `ern` used (`tokejsr`) | none | none |
