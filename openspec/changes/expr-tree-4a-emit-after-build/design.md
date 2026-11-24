# Design

## Context

The series and its check: `expr-tree-1-typing/design.md`. Enter/exit actions: `expr-tree-3c-calls/design.md`; the lookup without use: `expr-tree-3d-library-calls/design.md`; the side-effect table: `docs/blitz2/expression-tree.md` (written by changes 3a-3e).

## Goals / Non-Goals

**Goals:** emission after the build with today's bytes; no code patched, rewound or moved inside an expression.

**Non-Goals:** typing changes; `countpees`, look-aheads, assignment copies (change 9).

## Decisions

### 1. The unit: a top-level `maineval2`

A call of `maineval2` from a statement builds and emits one tree; a `maineval2` reached while building (a parameter, an index, a bracket) adds a subtree. The arena is marked at the top-level entry and released after its emission; reset per statement as before.

### 2. Flow typing and errors at build time

The type so far and every type error are computed while the tree is built, in reading order (step 1's function); emission raises only the errors that today come from emission itself (none known; task 1.2 checks). This keeps "the first of two errors" (spec).

### 3. Constant fold on nodes, value by emit-and-run

A fold is decided when an operator's two operands are constant nodes (today's pairwise rule). The value is still obtained by emitting the operation into the buffer and running it (`callregs`), so FFP and Quick values and the linked libraries are today's; `lasta6` is restored; `fraclitdrop` (#1186) and the relocation cleanup (#1049) stay; no fold while `nomemleft`. Alternative (computing on the host side in C): other values possible for FFP; not byte-identical; a named step 6 item at most.

### 4. The dropped conversion keeps its library

`cmpownleft` rewinds a conversion `varcont` emitted for a one-element left operand of a comparison, frees its relocations and restores `lasta6`, but a library the conversion's routine linked stays linked. In the tree the conversion is not emitted at all; the node carries "dropped conversion via routine R" and its enter action makes the use of R's library (`uselib3`) without code. A test source whose only use of that library is such a conversion pins it (task 1.1).

### 5. Arena blocks

Further blocks of 4 KB by `AllocMem` when the static block is full, freed at the statement reset. The size per node and the worst case are measured (task 2.5). Failure: the error of open question 1, without retry.

## Risks / Trade-offs

- [A side effect at the wrong action] -> bytes differ in the check; the side-effect table names where each belongs.
- [Memory: up to about 128 KB for a line that fills the 8 KB macro buffer (analysis estimate)] -> blocks are freed per statement; measured in 2.5.
- [Compile time] -> measured (`corpus_cd` duration before and after), reported; no budget exists for the compiler's speed.

## Planning choices (mine, not the user's)

- Decision 3: `Point: fold by emit-and-run on nodes — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor`.
- Decision 4: `Point: a library linked only by a dropped conversion goes missing — L2 Possible / I2 Medium / E2 Reasoned → Plan` (the coordinator's recorded Plan choice; task 1.1 is its test).
- Arena error: rated `L1 Unlikely / I3 Observable / E2 Reasoned`, but D-018 (out-of-memory fixes) bears on it as an answer for another case: Decide (proposal, open question 1).

## Findings (implementation)

- Task 1.1's source was built by hand: `r.f = (q = 2)` with `q` a Quick. The Float that `r` wants makes `varcont` convert `q` with floatquicklib's routine (`qtof2`, `qutofl`, whose call loads ffplib's base: ffplib is first used there too); the comparison drops the conversion (`cmpownleft`, a Quick to Float), and the comparison's Word is converted to the Float by ffplib (`wordtofloat`), so floatquicklib is linked by the dropped conversion alone. Checked on main with AB3 mode's command line compiler (`blitz2-ab3 -s`): the program is 148 bytes longer than the same program with `r.f = -1`, and adding a conversion `f.f = q` to both makes them 16 bytes apart (its code only). The test is `tests/blitz2/expr-build.test.bas` (case `dropconv`, with and without runtime error checking, `blitz2_expr_build_prev`), with the fold cases of task 1.3.
- Task 1.2: `tests/blitz2/expr-errors.test.bas` and its data sources (`expr-errors.test/`): the spec's `a$*2+nofunc{}` is "Type Mismatch" on main (the `2` under the type so far String, `consttype2`, before the operator), so the operator's error is `a$*a$` ("Illegal Operator for Type", raised by `doop` when the operator is emitted, before the next element is read); a missing bracket after both, an error in the first parameter of a Function and of a library function before an unknown Function after it, a String for a Word parameter ("Can't Convert Types", at the end of the parameter) and a division of two constants by zero before an unknown Function ("Division by Zero", raised by the fold's check when the operator is emitted).
- Task 1.3's fold while the object buffer is full: `tests/blitz2/expr-fold-full.test.bas`, after the pattern of `compiler-fold-buffer-full` (#1190): a child compiled with an object buffer of 2992 bytes, which 280 folds fill before the folds of a Mod (modlib's first use), a sum after a variable, a negated bracket, a Float and a Quick product and `Not` of a sum with a division.
