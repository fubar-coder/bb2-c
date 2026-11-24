# Design

## Context

After `compare-compound-left` the expression is a typed tree before emission (flow typing at build time, `expr-tree-4a-emit-after-build`); step 1's function decides each operation from (operator, left, right, facts). Variant C of #1178 and its rounding ("Zwischenlösung: / gegen Null (Recommended)") are the value rules of a mixed operation. The series: `openspec/changes/archive/2026-11-24-expr-tree-1-typing/design.md`.

## Goals / Non-Goals

**Goals:** typing by operands; the changed results listed exactly by the model before the implementation.

**Non-Goals:** step 6 optimisations.

## Decisions

### 1. Typing pass after the build, errors still at build time

The flow typing at build time stays for the errors (the first of two errors, spec `compiler/expression-limits`); a second pass over the finished tree (iterative along chains) computes the operand types and the conversions; emission uses them. Where operand typing would make a source that compiled refuse (or the other way), that is a named change of its own (task 2.2 lists such cases; expected none).

### 2. The model first

The expression matrix's model (`tools/lib/exprmatrix/model.c`) gets the decided rule as a third rule set; the cells whose value changes are the named cells. The 376 places of #1178 are re-run with the same rule. The implementation is done when the compiler gives the model's values for every cell.

### 3. Readable tables of today's rules first, generated and checked

Decision (user, 2026-11-24, `~/.local/share/bb2-c-work/decisions/rounds/tree/decisions.md`, "expr-tree-5: readable tables of today's type rules first"). Suggestion: "Wäre es nicht sinnvoll die bestehenden Typ-Konvertierungsregeln, bzw. Operator-Ergebnis-Typen zu ermitteln und dokumentieren - unter Einbeziehung des Ziel-Datentyps, falls notwendig?"; answer to "als neue erste Aufgabe einplanen?": "Ja, mach das."

- **Tool**: logic in `tools/lib/exprtype_tables/`, command in `tools/`, host tests in `tools/tests/` (CLAUDE.md rule 6); `programs/blitz2/src/exprtype.c` is linked in, as `programs/blitz2/tests/host/test_exprtype.cpp` does. It is used again by Q4 (below) and by the last group.
- **How the wanted type enters** (to be confirmed by task 1.1 before coding): the wanted type is the initial type so far (destination, parameter, field or element type). `exprtype_element` converts each fetched element up to the type so far, `exprtype_binop` decides from the type so far after the left and after the right operand, so the result depends on the position in the chain, i.e. on the operand order. The tables make that visible: the key of a row is (operator, left type, right type, wanted type), and a chain position is a column group: the wanted type column shows the type so far at which the operand is met (the left operand of a chain starts at the wanted type, a later operand at what the chain gave so far). A note per group shows the same operator with swapped operands (`a+q*4` against `q*4+a`) and marks the rows where the two differ; that list is the order dependence the change removes.
- **Tables**: one per operator group (arithmetic `+ - * /`, bit operators `& | AND OR EOR`, shifts, `Mod`, power `^`, comparisons, the bit test/set operators), each cell giving the result type and the conversion of each operand (from -> to, a raw Quick cut). Exceptions are notes under the table with their issue: #1178 raw Quick, #1182 comparisons, #1188 constants with a fraction under Byte, #1209 bit operators under Quick, #978 sign-extended constants, #1179 Word operands of a later index. Constants and elements (`exprtype_element`, consttype2) are a table of their own: wanted type x constant facts -> type.
- **Where**: a generated page `docs/blitz2/expression-type-tables.md`, linked from `docs/blitz2/expression-types.md`, not an appendix of it. Why: the page is written entirely by the tool, so the check is a whole-file comparison without markers inside hand-written text, and the hand-written page keeps its explanations and measured examples.
- **Check**: in the build step that produces the page (a custom command/target of the docs, CLAUDE.md "Code conventions": a failed check means the result is not built, no stamp files); a test with deliberately broken input (a changed cell, a missing note) must fail it.
- **Q4**: each candidate rule (open question 1, including "constants as integral or fractional numbers") is printed by the same tool as the same tables beside today's, the cells that change and the CD places marked, in the decision report. After the decision the page is regenerated with the chosen rule in the implementation group; the changed cells are the named ones.

## Risks / Trade-offs

- [The changed set is larger than predicted] -> the matrix's named-cell list is the limit: a cell not on it that changes fails the matrix pass.
- [The rule is chosen without seeing the CD impact] -> the 376-place run and a CD count are shown with question 1.
- An option that changes results not intended and named (e.g. accepting `PREV_INTENDED=ON` without a list) would be I5 Critical; not proposed: the prev run's list is named.
