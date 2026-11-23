# Design

## Context

See `expr-tree-1-typing/design.md` ("Context", "The series"). `eval5` (`eval.c`): `pushprec(*op, prec)`, `regat++`, recursion for the right operand, `regat--`, `popprec`, conversion, `doop`, fold over `forthsp[-2..-1]`. `eval4` pushes a sentinel of precedence 1 per bracket and per `NOT`. #1080: 272 bytes of C stack per bracket level, about 490 per nested call; the compiler runs on a 48 KB stack of its own.

## Goals / Non-Goals

**Goals:** nodes and an arena that later changes can fill with element nodes; iterative emission along a chain; the limits as explicit, unchanged checks.

**Non-Goals:** deferred emission; dynamic memory.

## Decisions

### 1. Window, not whole tree

An operator node and its leaf operands are released when the operator's code is emitted (the moment `doop` runs today). The live part is the pending operators and their left operands: at most 64 operator and 32 element entries, the bounds of today's two stacks, so one static block covers it (size measured: task 1.3). Alternative: the whole expression from now on - needs dynamic blocks and deferred emission, which is change 8's subject; doing it here would move side effects before the elements are nodes.

### 2. The limits as counters at today's moments

`pushprec`'s overflow and `getelement2`'s `forthstack` overflow are counted as today (`forthstack` stays for the fold until change 8); `regat >= 7` stays in `eval5` before the right operand (#1185). A test source per limit pins message and line (task 1.1).

### 3. Iteration along the left spine

Today `eval5` loops for operators of equal or lower precedence and recurses for higher precedence and brackets; the node builder keeps exactly this shape (the loop becomes the iteration, the recursion stays where it is). A host test builds a chain of 100 000 equal-precedence operators in the arena code compiled for the host and checks that the walk's stack use does not grow with the length (the host build with a small thread stack, or the sanitizer's stack-use report).

## Risks / Trade-offs

- [The release point differs by one operator from today's emission point] -> bytes differ: the check catches it.
- [Frame size of `eval5` grows with the node bookkeeping] -> the #1080 depths measured again (task 2.3); the compiler's 48 KB stack has room (63 brackets measured 18683 bytes).

## Planning choices (mine, not the user's)

- `Point: window release keeps the static block sufficient; whole-tree arena deferred to change 8 — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
- `Point: stack measurement by a throw-away fill-pattern runner as in #1080 (report only, not a committed tool) — L2 Possible / I1 Low / E2 Reasoned → Monitor` (a permanent tool would be a C tool in `tools/`, rule 6, its own decision).

## Findings (implementation)

- Task 1.1's test is one of the BASIC test model (`tests/blitz2/expr-limits.test.bas` and `.test.toml`), not `expr-limits.bas`: `tests/README.md` asks a new test in this form, and a compile error is a data source (`expr-limits.test/NAME.bas`, action `basic-compile`) whose result the configuration asserts (`data.NAME == "error LINE MESSAGE"`). Seven cases compile at the limits; six data sources are one step beyond (64 parentheses, 65 entries from `+`, `*`, a parenthesis, `NOT` and 60 parentheses, 32 nested calls, an operand beyond d7, 30 nested `Right$`, 64 times `-(`).
- A `NOT` in a macro gives "Syntax Error" (not looked into; the test puts its `NOT`s in the line); 64 `NOT`s in a row do not fit a TED line, so the 64 pending entries mix `+`, `*`, a parenthesis, `NOT` and parentheses. In AB3 mode `NOT NOT b` and `NOT (NOT b)` give `NOT b` (observed with `blitz2-ab3`, not looked into); in BB2 mode both give `b`.
- Hooks instead of a walk with function pointers: `exprtree_climb` calls `exprtree_operand`, `exprtree_left`, `exprtree_done`, `exprtree_pre`, `exprtree_enter` and `exprtree_emit`, which `eval.c` defines (the host test its own), so no level of the recursion keeps a pointer to them; the current char is a field of the context (not a pointer to `eval4`'s local) and `eval2` gives the operator's index. The first form (a table of function pointers, the char by pointer, `eval2` giving the operator's address) measured 248 bytes per bracket level, this one 244.
- The frame: a bracket level is 244 bytes (before 224): `exprtree_climb`, `exprtree_operand` (with `eval2` and `getelement`), `element` and `eval4` replace `eval5`, `getelement2` and `eval4`. `operate`, `constfold` and `divzerochk` are no longer marked not inlined: they run in `exprtree_emit`, after the right operand, so their frames are not part of the recursion (the stack figures did not change).
- A run of minus signs is one node and a loop in `negate` (the original recursed through `getelement2` once per sign, the port with 123 bytes per sign: 200 signs needed 24707 bytes, about 400 would overrun the 48 KB stack). Each further sign does what `getelement2` did before its `-` (`negnext` flips, the flags of #1177, #1178 and #1182 cleared, `elemcount`); then one `neg` or `SPNeg` per sign, innermost first, as before. `getelement2` reads the char and calls `element` with it.
- The arena's bound: 64 counted nodes, the 65th made before it is counted, one leaf or one run of minus signs per level of the walk (a level begins after a counted node): 129. The first size, 128, was one short: 64 times `-(` reached the full arena ("Expression too Complex") before the 65th count ("Precedence Stack Overflow"); the case is now in the test. Measured high water (scratch build with a counter, `blitz2-ab3`): 63 parentheses 65 nodes (64 counted), 65 entries mixed 67 (64), 31 calls with 32 parentheses 65 (64), 16 `Right$` 22 (20), 28 `Right$` 34 (32), the operand in d7 22 (14), the chain of 1000 terms 4 (2), 200 minus signs 2 (1), `chr_mem.ab3` 7 (5), 63 times `-(` 128 (64). Size: 136 nodes of 32 bytes on the 68000, 4352 bytes.
- The stack high-water mark of a compile (stkrun, `blitz2-ab3` with a 200000-byte stack; before = 60d25326f): 5 parentheses 2627 -> 2747 (+4.6 %), 10: 3747 -> 3967 (+5.9 %), 40: 10467 -> 11287 (+7.8 %), 63: 15619 -> 16899 (+8.2 %), 16 `Right$` 9851 -> 10179 (+3.3 %), `chr_mem.ab3` 3071 -> 3099 (+0.9 %), 63 parentheses with 31 calls and 28 `Right$` (the #1080 case) 22811 -> 24091 (+5.6 %), 65 entries mixed 15355 -> 16491 (+7.4 %), 63 times `-(` 22927 -> 24207 (+5.6 %), the chain of 1000 terms 1623 -> 1643 (flat), 200 minus signs 24707 -> 1643.
- `blitz2` 112352 -> 113108 bytes, `blitz2-ab3` 151216 -> 151972.
- The planning choices: the window release keeps one static block enough (bound 129 nodes, the block 136), as planned; the stack was measured with the #1080 runner (stkrun in a throw-away script of the scratchpad), not with a committed tool.
- The check (prev 60d25326f as `-DBLITZ2_PREV=<hash>`, no `PREV_INTENDED`): `prev_results` and the 92 `corpus_exprmatrix*` 93 passed; the corpus, `corpus_cd`, `corpus_cd_ab3` and their prev runs, 143 `bug_orig_*_prev`, 21 `blitz2_*_prev` (with `blitz2_expr_limits_prev`), 27 `regression_*_prev`, `compiler_prev`, `ab3_cli_prev_*`, the site examples, `ted_ab` and the seven `blitz2_expr_limits` cases 229 passed; `ctest --preset bug-orig -R compiler` 2 REPRODUCED (`compare_compound_left`, `panic_remtask`) as on main; `blitz2_exprtree`, `blitz2_exprtype` and `covsel` also with `-DHOST_SANITIZE=ON`. The oracle not run (no `ext_*` touched).
- Task 1.2's red phase: the host test was written together with `exprtree.c`, not against stubs first; that it fails without the code's properties was shown by two mutations of the code instead: the loop taking operators of equal precedence one level deeper (`>=`) fails the order of the emission and the chain of 100 000 operators (a stack spread of 7056 bytes against 1120, 128 nodes live against 22), an arena of 128 nodes fails 64 times `-(`.
