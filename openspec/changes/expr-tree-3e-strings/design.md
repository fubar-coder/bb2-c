# Design

## Context

A string in an expression follows a protocol that depends on the order of emission (`eval.c`): the first string element of an evaluation gets the work area (`makesbase`: `jsr workstart` at the first one of the statement, `sbasegot`; in a later evaluation of the statement a3 saved, `sbgot` 2); each string element copies its text behind a3 and pushes its length (`copstring`); `+` adds the two lengths on the stack (`addstrings`), a comparison calls `strcomp`; the end of the evaluation ends the string and makes its start the value (`maineval2`, `sbtolong`; AmiBlitz3's filler `ext_strpointer`) or restores a3 (`ststfix`). A string literal's text is written to the literal area (`data1`) while it is read (`litstring`, AmiBlitz3's escapes `ext_strescape`). The series: `expr-tree-1-typing/design.md`; the leaves: `expr-tree-3a-leaves`; the calls: `expr-tree-3c-calls`, `expr-tree-3d-library-calls`.

Already nodes before this change: a string variable (a variable's leaf, `ELK_VAR`, its code `stvar` through `variable2`), a string Function and a string library function (call nodes, `makesbase` in their enter actions), and every operator (the `EXN_BINOP` nodes of step 2, whose emit `operate` -> `doop` calls `addstrings` or the string comparisons). Left: the string literal, which reads and emits in one routine.

## Goals / Non-Goals

**Goals:** the string literal a leaf; the string variable's code from its leaf only; the side-effect rows of the string elements and the string operators for change 8.

**Non-Goals:** moving any side effect (the work area, the literal area, the lengths) to another moment; the assignment of strings (`dolet2`, change 9); the string library functions' forms (step 3d).

## Decisions

### 1. The string literal is a leaf, its text written at parse

`ELK_STRING`: the parse part reads the text and writes it to the literal area as it is read (its place in `value`, its length in `aux`); the emit part `litemit` writes the code from the leaf (`move.l #lit,a0` with its relocation, the length pushed, `copstring`). The work area (`makesbase`) is the first part of the leaf's code and stays before the text is read, its moment of before. Alternative: the text written at emit (the report's "literals at emit time", `tr-expr-tree-report.md` 3.1) - the parse part would have to keep the text or read the source twice, AmiBlitz3's escapes included. Within an expression only literals write the literal area (`calcstatic` and `calcmaxs` run at the end of the program), and the leaves are emitted in source order (a literal is never folded), so the order of the literals in the area is the same either way.

### 2. The string operators are the operator nodes of step 2

`+` and the six comparisons on strings are `EXN_BINOP` nodes whose left type (`tbefore`) is String; their emit is `operate` as for every operator (`exprtype_binop`'s handler String: `addstrings`, `doeqs` ... `doges`). No node kind or fact of their own: their code needs nothing beyond the node and the emitter's state (`regat`), and their protocol is that of their operands (each pushed a length). The side-effect table names it.

## Risks / Trade-offs

- [The literal's leaf adds a local to `element`'s frame, which every level of brackets holds] -> the stack measured, `litstring` inlined and not (findings).
- [Change 8 emits after the build: the work area of a literal would come after its text] -> the code and the literal area are apart; what differs is only which of two errors comes first when `workstart`'s library cannot be used (direct mode) and the literal is not closed. Named in the side-effect table for change 8.

## Planning choices (mine, not the user's)

- `Point: no design.md was planned for this change (proposal and tasks only); this file holds the decisions taken while implementing and the findings — L2 Possible / I1 Low / E1 Obvious → Plan`.
- `Point: the literal's text stays at parse, against the report's "literals at emit time" (decision 1) — L2 Possible / I2 Medium / E2 Reasoned → Plan` (change 8 can revisit; no byte depends on it).

## Findings (implementation)

- Task 1.1's forms: listed from the branches of `litstring`, `makesbase` (the first string of a statement, a later one: `move.l a3,-(a7)`), `stvar` (the address from the path or from a5/a4), `addstrings` (nested to d6 and, through `getelement`'s spill, beyond), the string comparisons (`exprtype_binop`'s compare path with String) and `maineval2`'s end (`sbtolong` with and without a saved a3, with a type word pushed), rather than with a coverage build, as in `expr-tree-3c-calls` and `expr-tree-3d-library-calls`. The new test `tests/blitz2/expr-strings.test.bas` (BASIC test model, without and with runtime error checking, its own prev pass `blitz2_expr_strings_prev`) compiles and runs each of them: literals (empty, odd and even length), variables (simple, a NewType field, an array element, one without memory), chains of `+` with 16 literals, 16 variables and functions, `+` nested to d6 and d7, the six comparisons with each kind of operand on either side, comparisons in arithmetic and as operands of `AND`, `OR` and `NOT` (also in `If`), strings in Function parameters (nested, two, a string expression in a numeric Function's parameter, a call as the right operand), and two string expressions in one statement (`Instr`, `Left$` with `Len`, `Mid$` with `Instr`). Compiled and not run: `NPrint` with string expressions and a comparison (the type word pushed), a string under a numeric type so far. AmiBlitz3's escapes in string expressions: `tests/ab3/cli/str_expr.ab3` (REF), with string parameters of a Function in registers (`ext_strpointer`'s filler) and of library functions.
- Decision 1 as built: `litstring` makes the leaf (`ELK_STRING`), gets the work area (`makesbase`, the leaf's `start` before it), reads the text into the literal area (`value` its place, `aux` its length; `litdata1` as before) and calls `litemit`, which replaces `makelit` (the place from the leaf). `stvar` takes the variable's leaf (`vtype`, `off`) and returns nothing: its `moveq #7,d2` is the leaf's type (`vartypeof`), its `reget` (no side effect) was not read by `variable2`, which loses the local it gave `stvar` for the type.
- Decision 2 as built: no code; the operator nodes of strings are named in `exprtree.h` and in the side-effect table of `docs/blitz2/expression-tree.md`, "Step 3e", with the end of an evaluation that holds a string (`sbtolong`, `ststfix`, AmiBlitz3's `ext_strpointer`).
- The bound of the arena stays 161, `EXPRTREE_NODES` 168 (7728 bytes): a literal's leaf lives while its text is read, which makes no node, so it takes an element leaf's place; no counter build was made (no node is added).
- The stack (the runner of step 2, `blitz2-ab3` with a 200000-byte stack, compile only, the programs run under `reqtrap`; main = 0f0b5005d, whose figures are those of `expr-tree-3d-library-calls`): with `litstring` not inlined the cases with a literal were 36-40 bytes deeper (63 parentheses around a literal 15543 -> 15583, 8 levels of `+` 3375 -> 3411, 8 times `Str$(Len(` 8515 -> 8551, 12 nested `Right$` with a literal 7219 -> 7255, a comparison of literals in 62 parentheses 15323 -> 15363); as built (inlined, as before) 4 bytes deeper in two cases and unchanged in the others: 63 parentheses around a literal 15543 -> 15547, the comparison in 62 parentheses 15323 -> 15327; 8 levels of `+` 3375, 8 times `Str$(Len(` 8515, 12 `Right$` 7219, 10 parentheses 3743, the #1080 case 22183, 31 behind minus signs 16227, 31 times `-f{` and 32 times `-(` 26335, the mixed bound case 26895. 30 nested `Abs` around `Len("x"+"y")` is "Expression too Complex" (33 elements) in both, 15319 -> 15323. Against main before step 2 (the #1080 case 22811) the figures stand; the programs' outputs are the same.
- `blitz2` 115716 -> 115804 bytes, `blitz2-ab3` 154604 -> 154692.
- The planning choices: no `design.md` was planned, this one was written with task 1.1; the literal's text stays at parse (decision 1), so the order of the literal area cannot change in this step; change 8 decides it together with the work area's moment (the second risk).
