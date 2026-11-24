# The expression evaluator restructured: from blitz2.s to the C functions

The original compiles an expression in one pass, emitting each element
while it reads it (`docs/blitz2/expression-types.md`); `blitz2` builds it
whole, typed in reading order, and writes its code after it (step 4a).
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
| 10479-10487 `varcont`: `ext.w Dn` | a Byte value is held as a Word | `exprtype_element` (`ext`), emitted by `contemit` |
| 10488-10498 `varcont` `.notabyte`, `.nob`, `.conv2` | the value converted up to the type so far, or the type so far raised to the value's type; a Quick under a Long cut (the port: held raw, #1178) | `exprtype_element` (`conv`, `result`, `raw`), emitted by `contemit` |
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
| 10471-10477 `variable` up to `bsr calcvar` | the variable (made at first sight) and its offset or path | `variable` (`ELK_VAR`; a name with `{` is a procedure call, `afunction`: step 3c) |
| 10478 `bsr variable2`, 10641-10682 `variable2` | the value into Dn; the type in d2 (`moveq #3`, `move.b #3`, `and #255`) | `variable2` from the leaf; the type: `vartypeof` at parse |
| 10479-10500 `varcont` | `ext.w` of a Byte, the conversion up to the type so far | the decisions `contleaf` (`exprtype_element`) at parse, the code `contemit`; `callcont` (both, on the call node of a procedure, OS or library call: steps 3c, 3d) |

What each element (and each step of a variable's path, step 3b: the
`VST_*` rows) changes besides its code, and when (change 8 relies on
this table: what is at parse happens while the expression is read, in
source order; what is at emit happens when its code is written):

| Kind | Elements | At parse | At emit | Children | Errors |
|---|---|---|---|---|---|
| `ELK_CONST` | decimal, `$`, `%` literals, `#constants`, `SizeOf`, `On`, `Off`, `Maximum`; in a constant expression a label's value and `"abcd"`; AB3's `@"..."` | `#constant`: AB3's `ext_constant`, the labels (`#__debug`); `Maximum`: the library used (`findtoke`, step 3d); the fold fact (`ELF_FOLD`: `forthstack`'s `constgot` is 0; not for `Maximum` and `@"..."`, which `element` reaches after its test); `asmtype` toggled for a label in a constant expression | `constat`/`constend`/`constval` (#978, #1179), `litat`..`litend` (#1186); `wordval` of the fold stack (`element`, after the code) | none | parse: "Syntax Error", "Numeric Over Flow", "Constant Not Found", "Type Not Found", "Offset not Found", "Type not yet defined", "Illegal Token" (an object without maximum), "Type Mismatch" (`consttype2`), "Can't Convert Types" (`@"..."` under a Quick); emit: "Type Mismatch" for a type without a move (not reached after `consttype2`) |
| `ELK_PI` | `Pi` | none | none | none | none |
| `ELK_VAR` | a variable, a NewType field, an array element, a pointer, a string variable | the variable made with its offset at first sight (`fetchvar`, in source order; a local one: `ELF_LOCAL`); its path read by `varparse` (step 3b): `lastoffset` (the last field), `flagmask` and `varmode` back after each index, each step emitted when it is read (the `VST_*` rows) | `varmode` from the leaf; AB3's `ext_evalvalue` after the value's code (as before: in reading order with emit right after parse; change 8 must keep that order); a string's base and `copstring` (`stvar`, step 3e); a conversion's library use (`convtype`: `qtof2` -> `tokejsr`); `wordval`, `bytevalend` (#978), `conv` (#1177), `rawq` (#1178) | the steps of its path (`EXN_STEP`), emitted and released before the leaf is made from the variable node | parse: `fetchvar`'s and `varparse`'s ("Mismatched Types", "Array not yet Dim'd", "Array is not a List", "Offset not Found", "Type not yet defined", "Syntax Error" ...) |
| `ELK_ADDROF` | `&var` | as `ELK_VAR` | `varmode`; AB3's `ext_evalvalue` before the code | as `ELK_VAR` | as `ELK_VAR` |
| `ELK_LABEL` | `?label` | the label made at first sight (`labfind`: in the label list from then on) | the reference to a label not defined yet (`labref`: its address `pc` + 2, the statement's line and procedure), the relocation (`addoff`) | none | parse: "Syntax Error", "Label has been used as a Constant", "Label reference out of context"; emit: "Out of Memory" (the reference) |
| `ELK_USED` | `Used Object` | the library used (`findtoke`, step 3d) | `usedcode` patched in place | none | parse: "Syntax Error", "Illegal Token" |
| `ELK_ADDR` | `Addr Object(n)` | the library used (`findtoke`) | `addrcode` patched in place | the index: its code (with the maximum check of runtime error checking and the shift) before the leaf (`getmaxel`) | parse: "Syntax Error", "Illegal Token", the index's |
| `ELK_ERRNUM` | `ErrNum` ("Ern": no name in the token list, no source text reaches it) | none | the library routine `ern` used (`tokejsr`) | none | none |
| `VST_LIST` | `a()`, a List array's current item | none | `listcode1` patched in place; with runtime error checking the library routine `currentchk` used (`tokejsr`) | none | parse, before it: "Array is not a List" |
| `VST_INDEX` | each index of an array `a(i,j)` | the index read and compiled before the step (`arreval`, `arrevalchk`: the side effects of its elements, at their moments) | after the first index `regat` one up; with runtime error checking the library routines `arrerr` (each index) and the size check's (`arrszchk`) used | the index (its code `aux`..`start`) | parse: the index's; after the step "Syntax Error" (a `,` missing) |
| `VST_ARRAY` | the end of an array's indexes | none | `regat` one down; `movea5a0` patched in place | none | parse, before it: "Syntax Error" (a `)` missing) |
| `VST_PFIX` | a pointer array's element followed by `\` | none | none | none | none |
| `VST_BASE` | a NewType variable followed by `\` or taken as a value | none | `movea5a0` patched in place | none | none |
| `VST_BROPEN`, `VST_BRCLOSE`, `VST_BRSCALE` | a field array's element `v\f[i]` | the index read and compiled between `VST_BROPEN` and `VST_BRCLOSE` (`eval`), `lastoffset` and `varmode` back after it | `addqa0` or `leaa0a0` patched in place (`VST_BROPEN`), `brchkcode` patched (`VST_BRCLOSE` with runtime error checking) | the index (`VST_BRCLOSE`: its code `aux`..`start`) | parse: "Mismatched Types" (a field that is no array) before `VST_BROPEN`, the index's and "Syntax Error" (a `]` missing) before `VST_BRCLOSE`, "Type not yet defined" (#974) before `VST_BRSCALE` |
| `VST_DEREF` | a pointer field followed by `\` (a BCPL pointer: `VSF_BCPL`) | none | `movea0a0` patched in place | none | none |
| `VST_END` | the end of a path | the offset `varparse` returns (`addd0a0d3`) | `addqa0` or `leaa0a0` patched in place | none | none |

## Step 3b: arrays, pointers and paths (`expr-tree-3b-arrays`)

`calcvar` read a variable's path (the indexes of an array, the fields
after `\`, the index of a field array `[ ]`, the pointers it follows) and
wrote its address code while it read it. Its parse part is `varparse`
(`parse.c`): each step of the path is a node (`struct exprnode` of kind
`EXN_STEP`, `op` its step kind `VST_*`, `exprtree.h`) with what its code
needs (`value`, `off`, its facts `VSF_*`, `ELF_LOCAL` for `varmode`); the
emit part `stepemit` writes the code from the step and the emitter's
state only (`regat`, the templates), and the step is released after it.
A step is made when its part of the path has been read, after its index:
the index is compiled into `D(regat)` while it is read, as every
expression (its code is the step's `aux`..`start`), so the step's code
follows it at the moment it did before. The path's end is the variable
node (`EXN_VAR`: the variable or its last field, its type, `calcvar`'s
type and offset), which `varparse` returns. `variable` and `ampersand2`
make it their element's leaf (`varleaf`); `calcvar` is the statements'
entry: it reads the node into its results and releases it. The code and
its moment are the same as before. A step lives only while it is emitted
and the variable node only from the end of its path, so they take the
place of an element's leaf: the bound of the arena stays 129 nodes (63
arrays nested in their indexes behind minus signs, `-a(-a(... -b))`; the
host test `blitz2_exprtree` pins it). `tests/blitz2/expr-arrays.test.bas`
compiles every form of a path.

| Original (`blitz2.s`) | What it did | Now |
|---|---|---|
| 8061 `addd0a0` | `addq` or `lea` of the offset d3 to a2; d3 clobbered by the `addq` | `addd0a0` (the code), `addd0a0d3` (the d3 it leaves, which `calcvar` returns as the offset) |
| 8170-8192 `calcvar` up to the list's code, 8209 `.nolrerr` up to `bra .normalar` | the array's test, `added` ("Array not yet Dim'd"), the `)` of a List array's current item ("Array is not a List") | `varparse` |
| 8193-8212 `calcvar`'s list code, `.nolrerr` | `listcode1`, `listcode2`, `currentchk`, `listarfix` | `stepemit` (`VST_LIST`) |
| 8216-8233 `.notalist`, `.knob` | the frame of the array, `arreval` of the first index | `varparse` |
| 8234-8243 `arrszchk`, `.noszchk`, `.domul` | the size check, the first index times the element size, `regat` up | `stepemit` (`VST_INDEX`, `VSF_FIRST`) |
| 8244-8263 `.arloop`, `.isglob` | the dimension's offset (a local array's: -8), its check (`zarrchk`, `arrerr`) | the offset: `varparse`; the check: `stepemit` (`VST_INDEX`, `VSF_CHECK`) |
| 8264-8273 `.noer1` | the next index (`arrevalchk`), `makeamul` | `arrevalchk`: `varparse`; `makeamul`: `stepemit` (`VST_INDEX`) |
| 8274-8292 `.ardone` | the `)`; `regat` down, the array's base plus the index into a2, a List array's item header skipped | the `)`: `varparse`; the code: `stepemit` (`VST_ARRAY`) |
| 8293-8312 `.normalar`, `.noppnt`, `.pmskip`, `.pfix` | the char after the array, the type (`$8000`, `$C000`), a pointer array's element followed | `varparse`; the pointer: `stepemit` (`VST_PFIX`) |
| 8313-8330 `.notarr`, `.notap` | "Array not yet Dim'd" for `(`, a simple variable or pointer, a NewType variable's address into a2 | `varparse`; the address: `stepemit` (`VST_BASE`) |
| 8331-8352 `.loopm1`, `.loop0` | a field after `\` ("Mismatched Types", "Offset not Found"), its offset added up, the `[` of a field array ("Mismatched Types") | `varparse` |
| 8353, 8358-8359 | the offset added to a2, a2 saved | `stepemit` (`VST_BROPEN`) |
| 8355-8357, 8360-8364, 8368-8370 | the frame of the field array, the index (`eval`), the `]` | `varparse` |
| 8366-8367, 8371-8395 up to `.nobchk` | a2 back, the bound check (`brchkcode`) | `stepemit` (`VST_BRCLOSE`) |
| 8396-8399 `.nobchk` | the element size (a pointer: 4; "Type not yet defined", #974) | `varparse` |
| 8400-8403 `.dothemul` | the index times the element size added to a2 | `stepemit` (`VST_BRSCALE`) |
| 8404-8411, 8427-8430 `.realdone`, `.done3` | the char after `]`, a pointer field taken as a value | `varparse` |
| 8412-8426 `.pointer`, `.pskip`, `.pmore` | a pointer field followed: its pointer into a2, a BCPL pointer times 4 | `stepemit` (`VST_DEREF`) |
| 8431-8433 `.done` | the type of the last field | `varparse` |
| 8434 `.done2` | `bra addd0a0` | `stepemit` (`VST_END`); the offset: `addd0a0d3` |
| 8436-8441 `.simpvar`, `.simpvar2`, `.nopoint` | a simple variable's type and offset, a pointer's flag | `varparse` |
| 8170 `calcvar`'s results (d2, d3, a2, a3) | the type, the offset, the variable and its type | the variable node (`EXN_VAR`); `calcvar` for the statements, `varleaf` for `variable` and `ampersand2` |

What each step changes besides its code, and when: the `VST_*` rows of
the table of step 3a.

## Step 3c: procedure and OS calls (`expr-tree-3c-calls`)

A procedure call in an expression (`f{...}`, `afunction`) and an OS call
(`Name_(...)`, `amigalib`) is a node (`struct exprnode` of kind
`EXN_CALL`, `op` its call kind `CLK_PROC` or `CLK_OS`, `exprtree.h`),
made when the call has been looked up: the procedure (`findproc`) or the
OS library (`uselib`). Its code has two actions. The enter action runs
before the parameters: the registers below `regat` saved, the string base
(`makesbase`, a3 pushed in a string expression) for a procedure, d0/d1
and the parameter registers that hold a value of the expression (#344)
and a4/a5 (#958) for an OS call. The exit action runs after them: the
call, the result into `D(regat)`, the registers restored. Each parameter
is an expression of its own (its node `EXN_EXPR`), compiled and pushed as
it is read with the push protocol of before (`fetchprocps`, `peval`):
the node's children. Both actions run at the moments the code was written
before, so the code is the same; step 4a (`expr-tree-4a-emit-after-build`)
keeps them in this order when it emits after the build. After its exit
action the node is the leaf of the call's value (`callcont`: the
decisions and the code of `varcont`) and is released.

A call's node lives while its parameters are read, beside the run of
minus signs of its level. The compiler nests at most 32 calls (each holds
an entry of `forthstack` from its element on, the 33rd is "Expression too
Complex"), so the bound of the arena is 161 nodes: 64 counted nodes, the
65th, 64 runs of minus signs and 32 calls (32 times `-f{` and 32 times
`-(`; the host test `blitz2_exprtree` pins it); the arena has 168 (7728
bytes on the 68000). `tests/blitz2/expr-calls.test.bas` compiles the
forms of a call.

| Original (`blitz2.s`) | What it did | Now |
|---|---|---|
| 10522-10530 `afunction` up to `bmi illprocerr` | the procedure looked up ("Procedure not found", "Illegal Procedure Call"), its result type | `afunction` (the node: `value` the procedure, `own` its type) |
| 10531-10548, 10556, 10561 | the stack the procedure removes, `makesbase` for a string result, a3 pushed (`.sss`), the registers below `regat` saved (`savem`), `regat` kept and 0 for the parameters | `procenter` (the enter action) |
| 10557-10562 `.somepars` up to `bsr fetchpees2` | the parameters | `afunction` (`fetchprocps`) |
| 10550-10553, 10564-10567 | `chkstak`, the jsr (`makefjsr`), `regat` back; a call without parameters reads its `}` after the jsr | `procexit` (the exit action's first part) |
| 10569-10570 `.nopars` | the `}` ("Syntax Error") | `afunction` |
| 10572-10609 up to `.notst` | the result into `D(regat)`, the registers restored, a3 back (`deca3` or `geta3`, `dopulla3`), a string's length | `procresult` (the exit action's rest; AB3's `ext_evalvalue` of the result type there) |
| 10611-10613 `.notst` | the next char, `bra varcont` | `afunction`, `callcont` |
| 10183-10184 `amigalib` up to `bsr uselib` | the OS library's base used from here on | `amigalib` (the node, made by `function` after the lookup, step 3d: `value` the function's entry; `off` the base's offset) |
| 10185-10188 `bsr savereg`, `bsr pokemovem` | d0/d1 saved (`fuckpos`) | `osenter` (the enter action, with #344's and #958's registers) |
| 10189-10213 up to `bsr tstbras` | the brackets, the parameters (each pushed by `peval`, `stackuse` readjusted) | `amigalib` |
| 10214-10255 `.toend` up to `.nofin`'s `bsr pokemovem` | the parameters popped into their registers, `move.l off(a5),a6`, the jsr, the result, the registers restored | `osexit` (the exit action) |
| 10256-10258 | `reget`, `bra varcont` | `amigalib`, `callcont` |

What each call changes besides its code, and when (the columns of the table of
step 3a):

| Kind | Calls | At parse | At emit | Children | Errors |
|---|---|---|---|---|---|
| `CLK_PROC` | a Function in an expression, `f{...}` | the procedure looked up (`findproc`: only the procedures defined so far; a Function used before its definition is "Procedure not found"); AB3: the defaults of left-out parameters (`ext_procdefaults`), the argument list's depth (`ext_procargs`) | enter: `stackuse` less the stack the procedure removes (AB3's `ext_procstackbytes`); a string result: the string base (`makesbase`: `sbasegot`, `sbgot`, the library routine `workstart` used at its first use); in a string expression a3 pushed (`dopusha3`: the library 65235 used); `regat` 0 for the parameters. Exit: with parameters and runtime error checking the library routine `stchk` used (`chkstak`); the jsr's relocation (`addoff`); `regat` back; a3 back (`dopulla3`); AB3's `ext_evalvalue` of the result type; then `varcont`'s (`callcont`: `wordval`, `bytevalend`, `conv`, `rawq`, a conversion's library use) | each parameter (its expression, compiled into its register or pushed), between enter and exit | parse: "Procedure not found", "Illegal Procedure Call" before the node; the parameters' and "Syntax Error" (no `}`) between the exit action's parts |
| `CLK_OS` | an OS library function, `Name_(...)` | right after the lookup (`looktoke`) the token's library used (`calluse`, the use action of step 3d); the OS library used (`uselib`: its base variable made at its first use) | enter: none besides the code; exit: `libbase` and `libjsr` patched in place, `lasta6` -1 (a6 holds the OS library's base; fixed #1211, the original left it as it was); then `varcont`'s (`callcont`) | each parameter (pushed by `peval`, `stackuse` 4 less after it), between enter and exit | parse: the parameters' and "Syntax Error" (`tstbras`: the brackets, a missing parameter) before the exit action |

## Step 3d: library calls (`expr-tree-3d-library-calls`)

A library function in an expression (`Name(...)`, `libfunction`) is a
call node as a procedure call (`struct exprnode` of kind `EXN_CALL`, `op`
`CLK_LIB`, `exprtree.h`), made when its token has been looked up. The
lookup has no side effect (`looktoke`, `asmlibs.c`): the use of the
library (`uselib3`: copied into the program at its first use, its
variable offset) is the first part of the node's enter action
(`calluse`), right after the lookup and before anything of the call is
read, at the moment `findtoke` used it before. So the libraries keep
their order in the program and their offsets: a library first used in
the parameters of another one's function comes after it. An OS call's
node is made by the same lookup (`function`; then `amigalib`, step 3c).
The other callers of `findtoke` (the statements, `tokejsr`, `Used`,
`Addr`, `Maximum`, the library routines' jsrs `tokecode` and `maketjsr`)
keep the lookup and the use in one.

The rest of the enter action runs before the brackets (`libenter`: the
string base for a string result, the registers below `regat` saved, with
a3 in a string expression when the function loads a3, #46) and after
them (`libregs`: `regat` kept and 0, the type of a size specifier into
d0). The parameters are `dolibtoke`'s, which a statement still calls as
before: the form chosen by the number of parameters where it was chosen
(`libform`; the node holds it, so the parameters' wanted types are the
form's type bytes), the parameters each compiled into its register or
pushed as it is read (`libparams`, `fetchlibps`: the node's children,
with the type word of a parameter of unknown type pushed at its moment).
The exit action is the call (`libexit`: `libjsrcode`, the stack prepared,
the jsr or an intrinsic's code, the stack back) and, after the `)`, the
result into `D(regat)` and the registers restored (`libresult`). An
intrinsic armed for the call (`Int`, `Frac`, `Len`, `Asc`, the state
machine of `intrinsic_argpush`, `intrinsic_quick` and `intrinsic_code`)
is noted in the node (`prec`: its entry; `CLF_INLINE`: the call compiled
inline, its type then the entry's). After its exit action the node is
the leaf of the call's value (`callcont`). The bound of the arena stays
161: a library call is one of the at most 32 nested calls (10 times
`-Abs(` and 21 or 22 times `-f{` before 32 times `-(`: 160 and
"Precedence Stack Overflow" at 161). `tests/blitz2/expr-libcalls.test.bas`
compiles the forms of a library call.

| Original (`blitz2.s`) | What it did | Now |
|---|---|---|
| 6277-6299 `findtoke` up to `.found`, 6307-6316 `.userlib`, `.next` | the token's library and its entry | `looktoke`; `findtoke` is `looktoke` and the use |
| 6300-6305 `findtoke` `.found` | the library used (`uselib3`) | `findtoke`; for a library function or an OS call in an expression `calluse` (the node's use action, `function`) |
| 10087-10111 `function` | the special tokens, the lookup, the function test (`btst #1`), an OS call (`btst #2`) | `function` (the node: `value` the entry, `aux` the library) |
| 10112-10119 `libfunction` up to `sizespec2` | the result type, a size specifier | `libfunction` (`own`) |
| 10121-10128 `.no0`, `.notst0` | `makesbase` for a string result, `savem` | `libenter` (the enter action; `vtype` the movem mask, `CLF_A3`) |
| 10130-10138 up to `.nopars` | the brackets, `countpees` | `libfunction` |
| 10139-10148 `.skip0` up to `.no2` | `regat` kept and 0, `moveq #type,d0` for a size specifier | `libregs` (the enter action's rest; `reg`) |
| 10149 `.no2` | `bsr dolibtoke` | `libcallnode` (for a statement: `dolibtoke`) |
| 3184-3188, 3255-3258 `dolibtoke`, `.skip3` | the save of `numreps`, `userp`, `stackuse` and their restore | `dolibtoke`, `libcallnode` |
| 3189-3216 `dolibtoke` `.loop`, `.next` | the form with that number of parameters | `libform` (the node: `off`) |
| 3217-3228 `.found`, `.nopees` | the parameters (`fetchlibps`) | `libparams` |
| 3229-3254 `.skip` up to `.skip4` | `preplibst`, `movenumrep`, the jsr (`userjsr`), the stack back | `libjsrcode`; for a library function the node's exit action `libexit` |
| 10150-10154 | the `)` | `libfunction` |
| 10155-10180 `.skip` up to `.nomovem` | `regat` back, the result into `D(regat)`, the movem back, a string's length | `libresult` (the exit action's rest) |
| 10181 `.notst` | `bra varcont` | `callcont` |

What a library call changes besides its code, and when (the columns of
the table of step 3a):

| Kind | Calls | At parse | At emit | Children | Errors |
|---|---|---|---|---|---|
| `CLK_LIB` | a library function in an expression, `Name(...)`, `Name.t(...)` (a size specifier), `Name` (no parameters); the intrinsics `Int`, `Frac`, `Len`, `Asc` | right after the lookup (`looktoke`) the library used (`calluse`: `uselib3`, at its first use its code copied into the program, a maximum library's object variable, its variable offset; in a constant expression "Illegal Constant Expression" for a library without constant rules, in direct mode "Library not Available in Direct Mode"); AB3's `ext_newtoken` before the lookup; a size specifier read (`sizespec2`: `usertype`); the brackets counted (`countpees`); the intrinsic found (`intrfind`, `intrstrvar`'s look at the source) | enter: a string result: the string base (`makesbase`: `sbasegot`, `sbgot`, the library routine `workstart` used at its first use); `regat` 0 for the parameters. Exit: the form's code (`libjsrcode`: its `!libs` registers loaded, `toreg`, `uselib` of the libraries they name; with runtime error checking the error check routines; the jsr's relocation; or an intrinsic's code); `regat` back; then `varcont`'s (`callcont`: `wordval`, `bytevalend`, `conv`, `rawq`, a conversion's library use) | each parameter (its expression, compiled into its register or pushed, `stackuse`, `numreps`), between enter and exit; a plain string variable of `Len` or `Asc` read instead (`intrstrvar`, no parameter) | parse: "Token Not Found" (`looktoke`), "Syntax Error" (not a function) after the use; "Type Not Found", "Illegal Type" (a size specifier), "Syntax Error" (no parameters in brackets, no form with that number of them, no `)`), the parameters'; emit: "Only Available in Blitz mode" and "Only Available in Amiga mode" (`libjsrcode`) |

## Step 3e: strings (`expr-tree-3e-strings`)

A string in an expression follows a protocol of its own, which depends on
the order of the code: the first string element of an evaluation gets the
work area (`makesbase`: at the first one of a statement the library
routine `workstart`, `sbasegot`; in a later evaluation of the statement
a3 saved, `sbgot` 2), each string element copies its text behind a3 and
pushes its length (`copstring`), `+` adds the two lengths on the stack
(`addstrings`), a comparison calls `strcomp`, and the end of the
evaluation ends the string and makes its start the value (`maineval2`,
`sbtolong`) or restores a3. A string variable is a variable's leaf
(`ELK_VAR`, step 3a), whose code `stvar` writes from the leaf only; a
string Function or library function is a call node (steps 3c, 3d) whose
enter action gets the work area. The string literal is a leaf
(`ELK_STRING`): its work area is the first part of its code, before its
text is read, as before; its parse part reads the text and writes it to
the literal area (`data1`) as it reads it (AmiBlitz3's escapes with it,
`ext_strescape`), its place there and its length in the leaf; the rest of
its code (`litemit`) follows. The operators on strings are the operator
nodes of step 2 (`EXN_BINOP` with the left type String): `operate` emits
them as every operator, `exprtype_binop`'s handler String is
`addstrings` or a string comparison. So every part of the protocol keeps
its moment and the code is the same; step 4a
(`expr-tree-4a-emit-after-build`) emits the leaves and operators in the
same order. A literal's leaf lives while its text is read, which makes no
node: the bound of the arena stays 161. `tests/blitz2/expr-strings.test.bas`
and `tests/ab3/cli/str_expr.ab3` compile the forms of a string in an
expression.

| Original (`blitz2.s`) | What it did | Now |
|---|---|---|
| 10327 `litstring`'s `bsr makesbase` | the work area | `litstring`, before the text (the leaf's code begins: `start`) |
| 10328-10347 `litstring` up to `.skip` | the text read and written to `data1`, its length before it, `data1` even | `litstring` (the leaf `ELK_STRING`: `value` the place, `aux` the length) |
| 10348-10350 `bsr makelit`, `moveq #7,d2`, `bra getchar3` | the code, the type String, the next char | `litemit`; the type: `own` and `tafter` String; `litstring` reads the char |
| 10391 `makelit` | `move.l #lit,a0` with its relocation, the length pushed, `copstring` | `litemit` (the place from the leaf) |
| 10617-10639 `stvar` | the work area, the variable's address into a2, the length pushed, `copstring`; `moveq #7,d2`, `bra reget` | `stvar` from the leaf (`vtype`, `off`); the type: `vartypeof`; the char: `variable` |

What a string element and a string operator change besides their code,
and when (the columns of the table of step 3a):

| Kind | Elements | At parse | At emit | Children | Errors |
|---|---|---|---|---|---|
| `ELK_STRING` | a string literal `"..."` (in a constant expression: `"abcd"`, an `ELK_CONST`) | the text written to the literal area (`pokedata1`, `pokedata1b`: its length long, the bytes, the pad byte to even, #969; `litdata1`), in source order: within an expression no other code writes the literal area, and a literal is never folded; AmiBlitz3's escapes (`ext_strescape`: the bytes they stand for, `\\__CODELINE__` and the others) | before the text is read: the work area (`makesbase`: `sbasegot`, `sbgot`, `workstart` used at its first use, or `move.l a3,-(a7)`); after it the relocation of the text's address (`addoff`), the library routine `copstring` used (`tokejsr`); its length on the run-time stack (no `stackuse`: `maineval2` counts the one length left at the end) | none | parse: "No Terminating Quote" (`notqerr`), AmiBlitz3's escapes' (`ext_strescape`); emit, before the text: "Library not Available in Direct Mode" (`workstart`'s library). Change 8, which emits after the build, has the work area after the text: only the order of those two errors can differ |
| `ELK_VAR` of a string | a string variable, field or array element | as `ELK_VAR` | the work area (`makesbase`), the variable's address into a2 (unless its path left it there: `TF_ADDRINA2`), `copstring` used; its length on the run-time stack; under a numeric type so far its conversion (`stringtolong`: the length dropped, `sbtolong`) | as `ELK_VAR` | as `ELK_VAR` |
| `EXN_BINOP` of strings | `+`, `=`, `<>`, `<`, `<=`, `>`, `>=` with a String left operand | none (the operator read by `eval2`, as every one) | `+`: the right length popped and added to the left one (`addstrings`; at `regat` 6 the d6 that `getelement` saved dropped instead, #971); a comparison: the library routine `strcomp` used (`tokejsr`; at run time it removes both lengths and moves a3 back to the left string), then `Scc` (`cpoke`); the type so far String (`+`) or Word | its operands (each a string's code with its length pushed) | emit: "Illegal Operator for Type" (`doop`: any other operator on a String) |
| `EXN_EXPR` with strings | the end of an evaluation of a String, or of one with a string in it | none | a String: `clr.b (a3)+` and the start into `D(regat)` (`sbtolong`: the a3 saved on the stack, which the length then replaces, `sbgot` 2; else the work area's start, the variable of the library 65235, used), `stackuse` 4 up (when the type is pushed, `pushflag` > 0: the length popped, the type word pushed, `stackuse` 2 more, the length pushed again); AmiBlitz3's filler below a procedure's string argument in a register (`ext_strpointer`: `pea 0`, `stackuse` 4 up); another type with a3 saved: the saved a3 dropped (`ststfix`); `sbgot` back | the expression | none |

## Step 4a: built, then emitted (`expr-tree-4a-emit-after-build`)

An expression is built whole before its code is written. A `maineval2`
outside an expression begins a unit (`exprtree_begin`): the expression is
read, typed (in reading order: the type decisions and their errors are
those of steps 1-3e), its variables and labels are made, its libraries are
used and its literal texts are written, and every part of its code is an
action (`struct expract`, `exprtree.h`) appended to the unit's list in the
order the original wrote that code. When the expression is built, the
evaluation that began the unit runs the actions (`exprtree_run`); a
`maineval2` inside an expression (a parameter, an index) adds its part to
the unit: its enter action (`evalenter`: the stack use, the push flag, no
string base pushed yet) and its exit action (`evalexit`: the conversion to
the type asked for, the type word, the end of a string, the push of the
result). The list is the walk of the tree in post-order with each node's
enter actions before its children and its exit actions after them; a
node's actions and those of its children are one stretch of the list. An
action writes its code from its node and the emitter's state (`pc`,
`lasta6`, `sbgot`, `stackuse`, the facts the code
before it left: `constat`, `conv`); `regat`,
`varmode` and `evaldepth` are set to what they were where its part was
read. Shared code that a statement calls as well (a parameter pushed:
`pushap`, a path's step: `stepemit`, a procedure's default arguments,
`getmaxel`'s index scaling) records its code with `emitact`, which runs it
at once outside an expression. The nodes of the unit stay in the arena until
its end (`exprtree_unwind`); outside a unit a released node is free at
once.

What the original decided from the code it had written, the build decides
from what it has read and recorded:

| Decision | Original | Now |
|---|---|---|
| a constant fold (#1049, #1190) | `forthstack`'s `constgot` of the two operands, `nocando` in `dopow`, `domodl`, `domodf`, `mixedop`; the fold ran from the left operand's `forthstack` entry (its code's position, the relocations and `lasta6` then) | the build's forthstack (`bforth`: `constgot`, `wordval` per pending element) gives the BINOP `BNF_FOLD`, the nocando cases `BNF_NOFOLD`; the value is still computed by running the code when it is written (`constfold`), from the start of the left operand's code, whose first action (`startmark`) keeps the relocations and `lasta6` of that place; the emitter has no forthstack |
| "Division by Zero" of two constants | `divzerochk` when the operator is emitted | `divzerobuild`: the operands' actions and the operation up to the check are run behind `pc` while the expression is built, at that moment, and everything they changed is set back |
| a Word value of a later index (#1179) | `forthent.wordval`, set by `varcont` and the constants' code (`iswordconst`) | `bforth.wordval` from `contleaf` (`ELF_WORD`) and `constword` (a `move.l` of 0..32767), the BINOP's `BNF_WORDS` |
| a raw Quick (#1178), a lowered comparison (#1182) | `rawq` set by `varcont`'s code | `rawq` set when the value's code is recorded (`contrecord`), the BINOP's `BNF_LRAW`, `BNF_RRAW`, `BNF_RCMP` |
| a comparison's left operand gets its own type back (#1177, #1186) | `cmpownleft`: the operand's code ends in the conversion `varcont` made (`conv.end`): the conversion's code rewound with its relocations, `lasta6` set back; or it is a literal made a whole number with only its `neg.w` after it (`fraclitfloat`): rewritten as its FFP value | `ownleft`: the conversion's action is the last one that writes code since the operand began (`bconv`): the conversion is not written (`ELF_DROP`), the relocations of the libraries its routine used first are freed (`relocforget`), the libraries stay (decision 4); or the literal's is with only minus signs of a Byte or Word after it (`blit`): the literal writes its FFP value with the signs in it (`ELF_FFP`), the minus signs nothing (`EXF_ABSORBED`) |
| #978: an equality comparison of one byte value with one constant from 128 to 255 | `bytevalend`, `isbyteconst` of the written code, `byteconstext` patched the constant | the operands' facts when they are read (`exprtree_left`: the action of a value ending in the byte's `ext.w`, or of the constant, is the operand's last or only code: `EXF_BYTEVAL`, `EXF_BYTECONST`), `exprtype_binop` in `binopbuild`; the constant writes the sign-extended byte (`ELF_SIGNEXT`, `constemit`) |
| the string base (`sbasegot`) | set by `makesbase`'s code | `bsbase`: got before the unit, or a `makesbase` recorded in it |
| a later array index without runtime error checking (#1179) | evaluated as a Word (`sharreval`): a constant is a `move.w`, one Byte or Word element has no `ext.l` | evaluated as a Long and narrowed to its low word where its code is the original's: `idxnarrow` in `evalbuild` decides when the index is built, from the build's records: the constant that is the index's only code (`bcst`) gets `ELF_NARROW` (`constemit` writes the `move.w`), the conversion of one Byte, Word or Quick element that is its last code (`bconv`) gets `ELF_NARROW` (`contcode` writes it without the `ext.l`), the rounding of a raw Quick (`rawq`) is recorded without it (`rawcutact`) |
| an intrinsic compiled inline | `intrinsic_code` in `libjsrcode` (`INTR_DONE`) | `intrstate` is `INTR_QUICK` after the argument is built (`maineval2`, `intrinsic_quick`); `libexit` hands it to `libjsrcode` |
| the errors of the code | raised where the code was written: "Can't Convert Types" (`noconerr` in `convtable`), "Illegal Operator for Type" (`doop`, `negate`, `notit`), "Type Mismatch" (`constemit`), "Only Available in Blitz mode" and "Only Available in Amiga mode" (`makelsub3`) | raised where the code is recorded, in the same order (`convprep`, `opprep`, `binopprep`, `tokuse`, `userjsruse`) |

The libraries are used while the expression is built, at the moment the
original used them, because their variables share `varoff` with the
variables an expression makes at first sight and their code is copied into
the library area in the order of first use: a library a part of the code
calls is used where that part is recorded (`tokuse`, `userjsruse`: what
`tokejsr` and `userjsr` use, with the libraries of their register lists,
the blitz mode flag and its check routines; `convprep`, `opprep`: the
routines of a conversion and of an operator's handler), a function's
library right after its lookup (`calluse`), `Used`, `Addr`, `Maximum`'s
and an OS library at their reading (`findtokeact`, `uselibact`). A
library's relocations go into the program's list where its code first
calls it (`uselib3` keeps them while the expression is built and moves
them into the list at the library's first `uselib3` while the code is
written, `asmlibs.c`), which is where the original added them: the list
has the original's order. The relocations of a library whose calling code
is never written (the conversion a comparison drops) are freed, as
the original's `cmpownleft` freed them. A constant computed in the middle of an
expression (`evalconst`: a `~` constant, `CNIF`) is a unit of its own,
whose code is written and run at once; the build's records of the last
elements start again behind it.

| Original (`blitz2.s`) | What it did | Now |
|---|---|---|
| 8879-8962 `maineval2` | the evaluation: the saves, `eval3`, the conversion, the pushes, the restores | `maineval2` (a unit outside an expression), `evalbuild`, the actions `evalenter`, `rawcutact`, `evalexit` |
| 8968 `eval3`'s `sub.l #18,forthsp` | the expression's forthstack entry dropped | `eval3` (the build's forthstack) |
| 8983-9083 `eval5` | the operand, the operator's conversions and handler, the fold | `exprtree_climb` with the hooks: `exprtree_operand` (`startmark`), `exprtree_left` (`endmark`, `cutact`), `exprtree_pre` (`ownleft`), `exprtree_emit` (`binopbuild`; the action `binopact`: `operate`, `constfold`) |
| 9741-9752 `getelement2`'s forthstack entry | the element's code position, relocations and `lasta6`; `constgot` | `element` (`bforth`); the position, relocations and `lasta6` of the left operand of a fold: `startmark` |
| 9643 `getelement`'s spill | `move.x d6,-(a7)` before, `move.x d6,d7`, `move.x (a7)+,d6` after | `pokeact` actions |
| 9667 `negate`, 9688 `notit` | the `neg`s, the `not` | the actions `negemit`, `notemit` |
| 10860 `fetchnum3`, 9700 `fetchpi`, 9941 `doern`, 9993 `doused`, 10069 `doaddr`, 9915 `ampersand2`, 10409 `qmark`, 10478 `variable`, 10391 `makelit`, 10617 `stvar` | the elements' code | the actions `constemit`, `piemit`, `errnumemit`, `usedemit`, `addremit`, `addrofemit`, `labelemit`, `varemit`, `litbase` and `litemit`, `contemit` (a call's value) |
| 10183 `amigalib`, 10112 `libfunction`, 10522 `afunction` | the calls' code | the actions `osenter`, `osexit`, `stackact`; `libenter`, `pokeact` (a size specifier), `stacksave`, `libexit`, `stackrestore`, `libresult`; `procenter`, `procexit`, `procresult` |
| 8170 `calcvar`'s code | the path's steps | `stepdone` records `stepemit` (regat counted down and up by the build) |
| 8809 `arrevalchk` | a later index narrowed | `idxnarrow` (`evalbuild`), the leaves' `ELF_NARROW`, `rawcutact` |
| 5679 `pushap`, 5698 `handlevara`, 5742 `sendtype`, 5759 `baseadd`, 5781 `highadd` | a parameter's push | the actions `pushapact`, `ptypeact`, `pokeact`, `highaddact` |
| 12196 `getmaxel` | the index of an object scaled | the action `maxelact` |
| 3217 `dolibtoke` (`libparams`) | `stackuse` 0 for the parameters | the action `stackzero` |
| AmiBlitz3 (`ab3proc.c`) | the defaults of omitted parameters, the argument lists' depth | the actions `defaultsact`, `argsact` |

## Step 4b: decisions on the nodes (`expr-tree-4b-lookaheads`)

The narrowing of a later array index is decided when the index is built
(`idxnarrow`, table of step 4a); the code of `Select`'s expression is
written in its place:

| Original (`blitz2.s`) | What it did | Now |
|---|---|---|
| 13183-13192 `doselect`'s `pushstart`, `pushdo`, `clr`, `pushput` | the expression's code cut out, the `clr.l -(a7)` of a string written, the code put back behind it and its relocations moved | `bakevalubefore`: the expression built (`evalunit`), the `clr` written (`selclr`), then the expression's code |

Not decided on the nodes: `countpees` counts the parameters of a library
function in the source before they are read (its form, chosen by their
number, gives each parameter the type it is read in); the intrinsics
`Frac` and `Len`/`Asc` look at the source behind the call and at the
argument (`fracend`, `intrstrvar`); `dolet2` moves the code of the left
side's address behind the right side (`pushstart`, `pushdo`,
`pushput`; the open original bug `compiler-let-left-a6`).

## Node bound

`EXPRTREE_NODES` (`programs/blitz2/src/exprtree.h`) is 168 nodes, 7728
bytes on the 68000 (a node is 46 bytes). The bound of the nodes live at
once grew with the steps, each measured by compiling nested forms and
reading the host test's `maxlive`:

| Step | What lives besides the counted nodes | Bound | Reached by |
|---|---|---|---|
| 2 | the 64 counted nodes, the 65th (made before it is counted), one leaf or one run of minus signs per level | 129 | 64 times `-(` ("Precedence Stack Overflow"); 63 times `-(` compile with 128 nodes live |
| 3a | an element's leaf lives only while it is emitted: behind the run of minus signs of the innermost level it is the 129th, never with the 65th counted node | 129 | 63 times `-(` and `-b` |
| 3b | a step of a variable's path and the variable node take the leaf's place (made after the index, released before the next node) | 129 | 62 times `-(` and `-a(-c)`; 63 arrays nested in their indexes, `-a(-a(... -b))` |
| 3c | a call's node lives beside the run of minus signs of its level while its parameters are read; the compiler nests at most 32 calls (each holds an entry of `forthstack`, the 33rd is "Expression too Complex") | 161 | 32 times `-f{` and 32 times `-(` ("Precedence Stack Overflow") |
| 3d | a library function's call is such a node and counts among the 32 | 161 | 10 times `-Abs(` and 21 or 22 times `-f{` before 32 times `-(`: 160 and 161 |
| 3e | a string literal's leaf lives while its text is read and makes no node | 161 | unchanged |
| 4a | the whole expression stays until its code is written: its nodes and the actions of its code (an action is 24 bytes on the 68000) | grows with the expression | a sum of 1000 terms `kk+kk+...` 280812 bytes (67 further blocks, about 280 bytes per term), a line of 8 KB from macros (2700 terms) 758222 bytes (184 blocks); a usual line's expression fits the static block (`b.l = a(x+1)*3+f{x,y*2}-Abs(q*4)+Len(s$+"cd")`: 4940 bytes); 63 parentheses 9144 bytes, the #1080 case 25768 bytes |

The arena has a margin of 7 nodes to 168 (it was 136 with the 129 of
step 2). Since step 4a the static block is the first of the arena's
blocks; what it cannot hold goes into further blocks of 4 KB from the
system (`EXPRTREE_BLOCK`), kept for the next nodes until the statement
ends; when the system has none, the compile stops with "Out of Memory"
(`exprtree_nomem`, TED's notice, #369), without a retry. The figures
were measured with a throw-away build of `blitz2-ab3` that printed the
arena's high water.
