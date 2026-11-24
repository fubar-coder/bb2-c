# Proposal

## Why

Eighth change of the expression-tree series (#1180 step 4, first part; the series: `expr-tree-1-typing/design.md`). After changes 2-7 every element and operator is a node, but each is still emitted as soon as it is read. Step 5 (`base+q*4` = `q*4+base`) needs the whole expression before any code. This change builds each expression whole, types it by today's flow rule while it is built (so types and errors come in reading order, as today), and emits it afterwards. It removes the single-pass mechanisms that patch or rewind emitted code: the "forth" stack of the constant fold, the byte-constant patch (#978) and `cmpownleft`'s rewind (#1177). Its byte identity is the proof that the side effects are in today's order before step 5 changes any type.

## What Changes

- **Build, then emit**: a top-level `maineval2` builds its expression (parameters, indexes and brackets as subtrees), types it by flow at build time, then emits it in post-order with the enter/exit actions of `expr-tree-3c-calls`. Variables and library uses are made at build time, at the place the original made them (a library's first use takes its variable offset from the same counter as new variables, so the reading order keeps the offsets; design, "Findings"); relocations, label references and literals are written at emit time, each at its enter or exit action (a library's relocations are moved into the list at its first use while the code is written).
- **Arena with further blocks**: the static block of `expr-tree-2-operators` and, when it is full, further blocks from the system, freed at the end of the statement.
- **Constant fold decided on nodes**: an operator whose operands are both constant nodes is folded; the value is still computed by emitting and running the code (FFP and Quick values bit-identical), the library the folded code used stays linked (#1049), `lasta6` restored, no fold while the object buffer is full (`nomemleft`). `forthstack` goes.
- **#978 as a node rule**: an equality comparison of a byte value with a constant 128..255 emits the sign-extended constant directly; `isbyteconst`/`byteconstext`'s patch goes.
- **#1177's left operand as a node rule**: the left leaf of a comparison is emitted without the conversion `varcont` would have emitted; `cmpownleft`'s rewind goes. Where the dropped conversion was a routine call (`qtof2` -> `tokejsr`), its library is still used at that moment, so it stays linked as today (coordinator record 2026-11-22, the planning agent's Plan choice).
- The limits stay (spec `compiler/expression-limits`).

## Not in scope

`countpees`, the intrinsic state machine and text look-aheads, `pushdo`/`pushput` (`expr-tree-4b-lookaheads`); any type change (#1183: `compare-compound-left`; step 5); folding more than today's pairs (step 6, not planned).

## Results that change

None (rule 3); the check of the series. One new behaviour without a changed result in the compared sources: an expression whose tree does not fit into the memory left gives a compile error (open question 1).

## Capabilities

### New Capabilities

- `compiler/expression-limits`: how large and deep an expression the compiler accepts, which error it reports beyond that, and what it needs of memory and stack.

### Modified Capabilities

(none)

## Impact

`eval.c` (`maineval2`, `eval5`, `constfold`, `divzerochk`, `cmpownleft` removed, `getelement2`, `varcont`), `libcall.c` (`isbyteconst`, `byteconstext`, `consttype2`), `exprtree.c`, `compile.c` (statement reset), docs (`docs/blitz2/expression-tree.md`, `docs/architecture.md`, the #978 and #1177 rows of `docs/blitz2/deviations.md` where they describe the mechanism). After `expr-tree-3e-strings`.

## Open questions

1. (Answered 2026-11-24, rounds/tree/decisions.md "Q2: the arena without memory gives \"Out of Memory\"") The error when the arena cannot get memory: "Out of Memory" (the compiler's existing error, with TED's notice, #369), not "Expression too Complex". The spec scenario follows the answer.
