# Proposal

## Why

No test says, for every combination of operand types, operator and context, what blitz2 computes. What exists covers single slices: `compiler-quick-compare` (comparisons with a Quick, #1177, 93 cases), `tests/blitz2/quick-arith-long.bas` (today's Quick arithmetic), the 15 measured rows of `docs/blitz2/expression-types.md`, the corpus programs `t01`/`t02`/`t11` (one type at a time) and `compiler-constdiv-*`. The coming work changes the typing of expressions: #1178 (Quick arithmetic, change `quick-long-arithmetic`), #1179 (array index type), #1182, then the expression tree #1180 (steps 1-4 byte-identical, step 5 order independence). Each of them needs a regression test that shows which cells change and that no other cell changes.

User decision (2026-11-22, `task-decisions.md`, "Order of the Quick work; matrix test"): "Ja, vor #1178 (Recommended)": a change of its own before #1178; the matrix type x operator x context x order measured with 2.10, 1.95 and AmiBlitz3; the expected value of each cell from the user's recorded rules, a Decide question where no rule applies; it becomes the regression test of #1178 and of the tree.

## What Changes

- **The matrix.** Operand kinds Byte, Word, Long, Quick, Float variables, a string where it applies (`+` and the six comparisons), literal constants of Byte, Word and Long size and with a fraction; binary operators `+ - * / MOD ^ AND OR EOR LSL LSR ASL ASR` and `= <> < <= > >=`; unary minus and `Not`; contexts destination Byte/Word/Long/Quick/Float, a Long/Word/Quick/Float parameter, `If`, the first and a later array index, `Print`; both operand orders. Each cell prints one line with a key and its value (raw bits for Quick and Float values).
- **Measurement first.** Every cell is run with the CD's 2.10, blitz2 1.95 (the build of `blitz2.s`), AmiBlitz3 v3.14.0, and the port with the CD's and with the C libraries, runtime error checking off (the index cells also on). The result is a cell table in the work directory with the values of each compiler.
- **Expected values from the recorded rules.** A typing and value model (C, `tools/lib`) gives each cell its expected value from the rules the user recorded (#1177, #1178 variant C with its rounding rule and refinement d, #1179 direction A, #1182, the measured Float and Quick conversions), or the original's value where it is the exact result of the operation in its type (D-003). Cells that no rule covers are collected by class as Decide questions; they are not committed until answered.
- **Cells of open bugs are marked.** Each cell gets a class: agreement (original = port = expected), fixed (port = expected, original differs: #1177 and earlier fixes), open (port = original, expected differs: #1178, #1179, #1182, #1183, or a new find), or question. Open cells go into bug-orig tests per bug (REPRODUCED until the fix, FIXED after it); agreement and fixed cells into a corpus pass that runs in `all` against the port and in `originals` against 2.10. Test form decided by the user (2026-11-22, `task-decisions.md`, "#1185 fix; matrix test form"): "A: Alte Form, später umziehen (Recommended)"; the tests move to the BASIC test model in round U4.
- **Generated sources.** A C tool in `tools/` generates the test sources and expected files from a committed matrix description and the model (CLAUDE.md rule 6); a hyphenated build target writes them, a host test checks that the committed files equal the generator's output.
- **AmiBlitz3 as an oracle.** The agreement and fixed sources become `ab3_cli` cases with `REF`, AmiBlitz3's own output as `REF_EXPECT` (its differences named in `docs/ab3/variant.md`).
- No compiler or library change. No result of the build changes (rule 3): only tests, a tool and documents are added.

## Not in scope

- Any fix (#1178, #1179, #1182, #1183, #1185, #1180 are their own changes). #1185 gets the compile error "Expression too Complex" (decided 2026-11-22), so it has no value cells: a cell at that register depth is a `noncompile` cell once that fix is in.
- Expressions of three or more operands, parentheses, nested calls; operands that are functions, array elements, NewType fields, pointers, `&var`, `?label`, `#constants`.
- The compiler's errors for invalid combinations: the measurement records which cells do not compile, the test sources leave them out.
- Runtime error checking except for the array index cells.
- Moving the tests to the BASIC test model (change `bug-orig-basic-tests`, round U4: the generator then writes the new form).
- Speed of the compiled code.

## Capabilities

### New Capabilities

- `testing/expression-matrix`: the systematic test of expression typing: the dimensions of the matrix, how each cell's expected value is derived and where its test lives, how open bugs are marked, and the comparison with the originals and AmiBlitz3.

### Modified Capabilities

(none)

## Impact

- New: `tools/exprmatrix/` (program), `tools/lib/exprmatrix/` (model and generator), `tools/tests/` (snitch tests); the matrix description and generated sources under `tests/corpus/tests-exprmatrix/` and `tests/bug-orig/compiler/` (`compiler-expr-*`), expected files in `tests/corpus/expect/`; `tests/ab3/cli/` cases.
- Edited: `tests/corpus/CMakeLists.txt` (a pass), `tests/ab3/CMakeLists.txt` (cases), `tools/CMakeLists.txt`, `tools/testinfo/rules.txt`, `tests/README.md`, `docs/blitz2/expression-types.md` (a pointer to the matrix), `docs/ab3/variant.md` (AmiBlitz3's differences).
- Follow-up in other changes: `quick-long-arithmetic` (#1178) adds `; @fixed #1178` to the matrix's #1178 tests and turns the affected corpus files into pairs; its tasks are updated after this change (task 6.2).
- Test time: new emulator sessions in `all` (the corpus pass, the open bug-orig tests only in `bug-orig`) and in `oracle`; their number depends on the cut (design, decision 2).
