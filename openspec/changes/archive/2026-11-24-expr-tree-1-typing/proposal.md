# Proposal

## Why

blitz2 compiles an expression in one pass and emits each element while it reads it; every type comes from one "type so far" (issue #1180, `docs/blitz2/expression-types.md`). Issue #1180 moves the expression compiler to a tree so that, in a later named step, the type of an operation depends on both operands and not on their order (`base+q*4` = `q*4+base`; `~/.local/share/bb2-c-work/decisions/rounds/quick/decisions.md` 2026-11-21 "#1178 direction: variant C; expression tree planned": "Und ja, base+q*4 muss das gleiche ergeben wie q*4+base, aber das machen wir in einem späteren Schritt"). The type decisions are today spread over `eval5`, `doop`, `compareop`, `cmplq`, `varcont` and `consttype2`, each taken at the moment of emission. This first change of the series collects them into one function that no emitter decides around, and pins it by host tests, so that the later steps move emission without touching a type decision.

This change is the first of eleven changes (the series, `design.md` "The series"). Order (recorded): `~/.local/share/bb2-c-work/decisions/rounds/quick/decisions.md` 2026-11-22 "Order of the Quick work; matrix test": "A: Fixes zuerst (Recommended)" -> #1178, #1179, #1182, (#1185), then tree steps 1-4 byte-identical, then #1183, then step 5; "Go for steps 1-7" (the plan shown: "... #1182; #1188, #1189, #1190; #1180 steps 1-4, #1183, step 5").

## What Changes

- **One type decision function** (new file `programs/blitz2/src/exprtype.c`, pure C without compiler globals): for a binary operator, its inputs are the operator, the type so far before the left operand (`left`), the type the right operand ended with, and the operand facts the merged fixes need (raw Quick of #1178, the byte value and byte constant of #978, a comparison of #1177, the Long-index rule of #1179 and the right-hand comparison rule of #1182, as merged when this change starts). Its result: the conversions of each operand, the type of the handler to call, the resulting type so far. A second function gives the element rule of `varcont` and `consttype2` (the type so far after an element of a given own type).
- `eval5`, `doop`, `compareop`, `cmplq`, `varcont` and `consttype2`'s sizing call these functions and only emit what they return. No emitted byte changes.
- **Host tests** (`programs/blitz2/tests/host`, snitch, one program per test file): the functions against a committed table of every operator x type so far x left type x right type x operand fact (the golden table), and against the expression matrix's model (`tools/lib/exprmatrix/model.c`) for the two-operand cells both define.
- **Comments and docs** in the form the user settles for restructured evaluator code (open question 1; `docs/blitz2/CONVENTIONS.md` asks for "same algorithms" and a `blitz2.s:LINE` reference per function).

## Not in scope

- Any node, tree or arena (change `expr-tree-2-operators`), any change of when code is emitted, any change of a type decision (step 5, `expr-tree-5-operand-typing`), any fix (#1183 is `compare-compound-left`).
- The patch mechanisms (constant fold, the #978 patch, `cmpownleft`'s rewind, `countpees`, look-aheads, `pushdo`/`pushput`): they stay until `expr-tree-4a-emit-after-build` and `expr-tree-4b-lookaheads`; the function only decides what they act on.
- The function optimizer (`compiler-function-optimizer`), not in parallel (same file).

## Results that change

None (CLAUDE.md rule 3): every executable, every compile error (message and line), `acidlibs`, `deflibs` and every library file byte-identical to the previous commit, checked by the series' check (`design.md`, "The check of every byte-identical step"). The program `blitz2` itself changes (rule 3 allows it; named in the commit as a rework).

## Capabilities

### New Capabilities

(none: a restructuring without a change of behaviour; `skip_specs`)

### Modified Capabilities

(none)

## Impact

- `programs/blitz2/src/eval.c` (`eval5`, `doop`, `compareop`, `cmplq`, `varcont`), `libcall.c` (`consttype2`'s sizing), new `exprtype.c`/`exprtype.h`, `programs/blitz2/CMakeLists.txt` (also `blitz2-ab3`, which links the same sources).
- `programs/blitz2/tests/host/` (new test, golden table), `tools/testinfo/rules.txt` (selection).
- `docs/blitz2/expression-types.md` (where the rules now live), the documentation form of open question 1.
- Starts after #1178 (`quick-long-arithmetic`), #1179, #1182, #1188, #1189 and #1190 are merged (recorded order); the function replays what is merged then.

## Open questions

1. (Answered 2026-11-22) How restructured evaluator code is documented, given `docs/blitz2/CONVENTIONS.md` ("fidelity beats elegance: same algorithms"; "Each function starts with `/* blitz2.s:LINE label ...`"). The user's answer: "(a) Mapping-Tabelle (Recommended)", option (a) of the planning report (`~/.local/share/bb2-c-work/planning-tree.md`, Q1): a new function begins with `/* no asm counterpart; replaces blitz2.s:NNN LABEL ... */` (the form of the #1177 and #978 code), the original routines and parts of them that moved are mapped in a table in the new `docs/blitz2/expression-tree.md` (asm label -> C function or node rule), and `docs/blitz2/CONVENTIONS.md` gets a short section, whose wording is shown to the user before it goes in (not part of this change's commit).
