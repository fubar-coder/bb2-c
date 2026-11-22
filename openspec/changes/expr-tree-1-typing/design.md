# Design

## Context

Today's evaluator (`programs/blitz2/src/eval.c`): `maineval2` -> `eval3` -> `eval4` -> `eval5` (precedence climbing; the left operand is emitted, then the right one into `D(regat+1)`, then the late conversion `convtypef(left, *type)`, then `doop`, then the constant fold). Merged since #1180 was written: #1177 (`cmpownleft` rewinds a conversion `varcont` emitted for a one-element left operand, `compareop`, `cmplq`), #1185 (`regat >= 7` gives "Expression too Complex" before the operator), #1186 (`fraclitdrop` in `constfold`), #1187 (`doif2`, statement level). Planned before this change: #1178 (raw-Quick flag, K3 cut placement; `quick-long-arithmetic` design decisions 2 and 6), #1179 (every later index a Long), #1182, #1188, #1189, #1190. Analysis: `~/.local/share/bb2-c-work/tr-expr-tree-report.md`.

## The series

Eleven changes, one after another: ten steps of the tree and #1183's fix (all edit `eval.c`; "Go for steps 1-7": "changes that edit programs/blitz2/src/eval.c one after another"). Each is implementable and testable alone; steps 1-4 change no result.

| # | Change | #1180 step | Results |
|---|---|---|---|
| 1 | `expr-tree-1-typing` | 1: one type decision function | none |
| 2 | `expr-tree-2-operators` | 2: operator, bracket, minus, `NOT` nodes; arena; iterative walks | none |
| 3 | `expr-tree-3a-leaves` | 3: constants, `#constants`, scalar variables and fields, `&`, `?`, `SizeOf`, `Pi`, `On`/`Off`, `Used`, `Maximum`, `Addr`, `ErrNum`, AB3's `ext_element` | none |
| 4 | `expr-tree-3b-arrays` | 3: arrays, pointers, list arrays, `\` paths (`calcvar`) | none |
| 5 | `expr-tree-3c-calls` | 3: procedure calls (`afunction`), OS calls (`amigalib`) | none |
| 6 | `expr-tree-3d-library-calls` | 3: library calls (`libfunction`, `dolibtoke`, `fetchlibps`), library use split from the lookup | none |
| 7 | `expr-tree-3e-strings` | 3: string elements and `+` on strings | none |
| 8 | `expr-tree-4a-emit-after-build` | 4: the expression built whole, typed by flow at build, emitted after; fold, #978 and `cmpownleft` as node rules | none (only "Out of Memory" for an expression the arena cannot hold, open question of that change) |
| 9 | `expr-tree-4b-lookaheads` | 4: `countpees`, the intrinsic state machine and text look-aheads, `pushdo`/`pushput` of assignments and `Select` | none |
| 10 | `compare-compound-left` | #1183 | named: comparisons with a compound left operand, `-q` on the left, a Long constant on the left under Float |
| 11 | `expr-tree-5-operand-typing` | 5 | named: the cells and programs whose value depended on the operand order; `NPrint 1+1+1+q` gives 4.5 |

Step 6 of #1180 (optimisations the tree allows) is not planned.

Steps 2-3 keep emission at today's moments: a node is built and emitted when today's code emits it, so they need no memory beyond a fixed window and move no side effect. Change 8 is the first that emits after the whole expression is built; its byte identity is the proof that the side effects are assigned correctly (variables created at build time, libraries, relocations, label references and literals at emit time), before step 5 changes any type.

## The check of every byte-identical step

Every change 1-9 of the table passes, in its worktree, before the merge:

1. `ctest --preset host` (also with `-DHOST_SANITIZE=ON` for the host tests it touched), a clean rebuild without warnings.
2. The prev build of the commit before it (`cmake --preset amiga -DBLITZ2_PREV=ON`, `tests/README.md` "The prev build") and without `PREV_INTENDED`: `prev_results`; the corpus passes with their prev configuration, including the 776 sources of the expression matrix (`corpus_exprmatrix_*`, `corpus_exprmatrix_runerr_*`); `corpus_cd_prev` and `corpus_cd_ab3_prev` (the 724 CD sources: result, line, message, size, executable hash); `compiler_prev` and the `bug_orig_*_prev` passes; `ab3_cli_prev_*`; the site examples; `ted_ab` `Cc-Pc`. No difference.
3. The outputs of the expression matrix unchanged (`corpus_exprmatrix*` pass against their committed expected files), `ctest --preset bug-orig -R compiler` with the same REPRODUCED/FIXED set as on main (`tools/results fix-scope` against `main`: no case changed).
4. `ctest --preset oracle -R ab3` where AB3 code paths (`ext_*`) are touched.

The coordinator runs `all`, `bug-orig` and `originals` once after the merge (D-010).

## Goals / Non-Goals

**Goals:** every type decision of a binary operation and of an element in two pure functions; the emitters only emit; host tests that fail on any changed decision before an emulator runs.

**Non-Goals:** changing when or what is emitted; the patch mechanisms; cost or speed of `blitz2` (no budget exists for the compiler's speed).

## Decisions

### 1. Two functions, pure, in a file of their own

`exprtype_binop(op, left, right, facts) -> {conv_left, conv_right, handler_type, result}` and `exprtype_element(typesofar, own, facts) -> {conv, result}`. No compiler global is read; the caller passes `regat`-independent facts (raw Quick, byte value, byte constant, comparison, index). Pure so that the host test can compile the file with the native compiler, as `ab3date.c` (`programs/blitz2/tests/host/CMakeLists.txt`). Alternative: a table in `eval.c` - not testable on the host without the compiler's globals.

The facts that are code-position facts today (`bytevalend`, `isbyteconst`, `conv` of `cmpownleft`) stay where they are in this change: the function receives "left is a byte value" and "right is a byte constant" as booleans that `eval5` computes as it does now. Turning them into node properties is change 8.

### 2. The golden table and its independent check

The table (`programs/blitz2/tests/host/exprtype.tsv`) lists every operator of `operators[]` x type so far (0, Byte..Float, String) x right type x each relevant fact. It is written once by a small generator mode of the test program from the new function, after the series' check has shown byte identity (so it records today's behaviour as proved on all sources), and committed. The host test compares function and table; a changed decision fails. Because the table comes from the function, it is cross-checked against an independent source: the expression matrix's model (`tools/lib/exprmatrix/model.c`, the original's and the fixed rules) for every two-operand cell whose type both define. Whether the model exposes the type so far in a form the test can call is E2 (task 1.1 checks it; if not, the cross-check uses the matrix's measured outputs through the model's value function).

Alternative considered: generate the table from the 1.95 assembler build by instrumenting it - costly, and the matrix model already encodes the measured behaviour.

### 3. The byte-identity guard is the prev comparison over all sources (#1181)

The analysis's warning ("74 programs are too thin") is answered by #1181 (closed): the CD passes, `tests/blitz2`, `tests/ab3/cli`, the compiler bug-orig and regression tests, the site examples and the matrix are compared at executable level.

### 4. Library kept linked by a dropped conversion

Coordinator record 2026-11-22 (planning agent's Plan choice, L2/I2/E2): "a tree task must keep a library linked when `cmpownleft` dropped the conversion that linked it (qtof2 -> tokejsr), in either order". In this change `cmpownleft` stays as it is; the function decides the conversion, `varcont` emits it, `cmpownleft` rewinds it, the library stays linked. The requirement is carried by change 8, which replaces the rewind; there it is a task with its own test.

## Risks / Trade-offs

- [The function misses a fact a merged fix introduced (e.g. K3's cut at fetch for a shift consumer)] -> the series' check fails on the source that shows it; the golden table is written only after it passes.
- [The golden table is circular] -> the matrix model cross-check (decision 2).
- [`blitz2.s:LINE` references stop being 1:1 for the evaluator] -> open question 1 (proposal).

## Planning choices (mine, not the user's)

- Slicing into the series above, one change after another: `Point: eleven changes in sequence instead of one change with step groups — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
- Emission at today's moments in steps 2-3, first deferral in change 8: `Point: byte identity of steps 2-3 does not prove deferred emission; change 8 proves it before step 5 — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
- Golden table plus matrix-model cross-check: `Point: the table pins a decision that is wrong today — L1 Unlikely / I1 Low / E2 Reasoned → Monitor` (step 1 changes no decision; wrong decisions are fixes of their own).
