# Proposal

## Why

The generated tests of the expression matrix are still in the old form
(`docs/testing-old-form.md`, section 3): `exprmatrix expect` writes `ID.bas`,
`ID.expect` (and `bug-orig-record` the `ID.orig`) for the bug-orig tests and
corpus sources with `NAME.out` files for `corpus_exprmatrix`, each one ctest
test and one emulator session. The user's condition for the move (the BASIC
test model "ohne Performance-Nachteile") can now be met: bundled runs
(changes `basic-test-bundle-*`) pool cases of several sources into one
session, and test-local data sources (`basic-compile`) let one test judge
many refused compiles in one session. The user decided to plan the move now
and implement it after `expr-tree-4b-lookaheads` and before
`expr-tree-5-operand-typing`, which uses the matrix to name the cells its
rule changes.

## Decisions of the user

Quoted from `~/.local/share/bb2-c-work/decisions/`:

- `rounds/tree/decisions.md`, "Matrix tests into the BASIC test model:
  planned" (2026-11-24): "Ja, bitte einplanen" — the generator to the model
  form, bundled, a measurement against the old form per the user's
  condition, the compile-only ones as data sources, `corpus_exprmatrix`
  with it, `docs/testing-old-form.md` and the TODO entry; slot after
  `expr-tree-4b`, before `expr-tree-5`.
- `archive/2026-11-23-tr/decisions.md`, "Group 4: split the two long
  bug-fix-workflow requirements; the old form \"for now\"": "Die
  Generator-Tests würde ich gerne auch in der neuen Form sehen, allerdings
  erst, wenn man mehrere Tests auf einmal ausführen kann. Und wenn der Start
  der Tests durch cmake ebenfalls gebündelt werden kann. Mit anderen Worten:
  Sofern die Tests ohne Performance-Nachteile in das neue Format übertragen
  werden können."
- Same file, "Bundling answers; hang-after stays at 8 s": quote (2) "Ein
  Test nach der neuen Methodik sollte im Idealfall nicht langsamer sein als
  die alten Tests. Wenn das Bündeln den Performance-Nachteil mitigiert, dann
  ist das Bündeln natürlich ein Vorteil über den man nachdenken sollte";
  quote (4), the originals side in bundles: "Vorerst nicht"; quote (5),
  `corpus_exprmatrix`: "Ja, würde nach der Umstellung dazugehören".
- Same file, "Group 4: four answers on the rename to `; @fixed-orig #N`",
  generator: "Generator writes new name (Recommended)" (for the tests that
  stay in the old form; after this change none does, and rule 4 prescribes
  `fixed-orig = N` for a model test).
- `rounds/quick/decisions.md`, "Last answers of the session", T1 for all
  fixes: "\"Ja für alle\" - zumindest solange bis die Test-Infrastruktur
  fertig umgebaut ist" (fixed matrix cells stay in their bug-orig tests).
  Whether it still holds after the rebuild is open question 1.
- D-002 (rule 6): the generator stays C (`tools/lib/exprmatrix`); no script.

## What Changes

- **`exprmatrix expect` writes the BASIC test model**: per source one
  `ID.test.bas` with one case and one named check per cell (the cell's key
  as the check's name), and its `ID.test.toml` with the settings
  (`runtime-errors` for the checked index sources), the original's per-cell
  values as declared deviations and `fixed-orig = N` where the issue is
  fixed. The old-form writer goes (`ID.bas`, `ID.expect`, the corpus `.bas`
  and `NAME.out` / `NAME.orig.out` / `NAME.fixed-N.out`).
- **The bug-orig tests with a run** (52 today, `compiler-expr-ISSUES-CTX-SET[-u|-c]-NN`)
  keep their IDs and directory and become model tests; their recorded
  `ID.orig` is replaced by generated deviations of `compiler=orig195`.
- **The compile-only tests** (36 bug-orig `compiler-expr-ISSUES-refused-*`,
  123 regression `compiler-expr-refused-*`) become a few tests with many
  data sources of the action `basic-compile` (one test per issue set, one
  regression test per operator), judged by `data.FILE`, as
  `tests/blitz2/expr-limits` does.
- **`corpus_exprmatrix`** (95 sources, `corpus_exprmatrix_NAME` and
  `corpus_exprmatrix_runerr_NAME` with `_originals`) becomes model tests in
  `tests/bug-orig/exprmatrix/` with its own registration (prefix
  `corpus_exprmatrix_`), the cells port and 2.10 with both library sets
  as the old passes had; `NAME.out` becomes the checks, the pair
  `NAME.orig.out` / `NAME.fixed-N.out` `fixed-orig = N` with the deviations.
- **Bundled**: the port side of the moved run tests joins the bundles of its
  registration; the originals side runs unbundled ("Vorerst nicht").
- **Measured** against the old form before the old files go: full runs of
  `all` and `originals` before and after; the move is finished only if the
  matrix's tests are not slower in either preset (open question 3 if they
  are).
- **Removed**, where nothing else uses them: the old files, the corpus
  pass loop of `tests-exprmatrix` and the `corpus_pass` option
  `NO_EXPECT_TARGETS`, `tests/corpus/expect/corpus_exprmatrix*`.
- **Docs**: `docs/testing-old-form.md` (section 3 and the counts),
  `tools/exprmatrix/README.md`, `tests/README.md` ("The expression
  matrix"), `TODO.md` ("Tests left in the old form"), `docs/ab3/variant.md`
  (the `@fixed-orig` mentions), `tools/testinfo/rules.txt`.

## Results that change

No result of the compilers, libraries or TED (rule 3): no compiler,
library or TED code is touched. Named: the executables of the matrix's test
programs change, because a test program is now a driver of the BASIC test
model (the test library's calls around the same expressions) instead of the
plain source; the prev comparison compiles each new driver with both
compilers, so it needs no `PREV_INTENDED`. The AmiBlitz3 cases
(`tests/ab3/cli/exprmatrix/`) stay byte-identical.

## Not in scope

- The model (`tools/lib/exprmatrix/model.c`), the classes
  (`classify.c`), the description `matrix.txt` and `measured.tsv`: no cell
  changes its expected value.
- The AmiBlitz3 cases of the matrix (`tests/ab3/cli/exprmatrix/`): written
  as today.
- Bundling the originals side; bundles of tests with data sources (the
  TODO entry "Bundles with test-local data and runs from the editor").
- The hand-written old-form tests (`compiler-expr-depth-prec`, the other 62
  compile-only tests, TED tests).
- An output check in the test library (the `Print` context is judged by
  `output`, design decision 3).

## Open questions

1. **T1 after the rebuild** (answered 2026-11-24: "(a) Keep them (Recommended)",
   `rounds/tree/decisions.md` "exprmatrix-model-tests: Q1 and Q2"): the bug-orig tests of fixed
   matrix cells keep their IDs (one model test per source, `fixed-orig = N`).
2. **The AmiBlitz3 equivalence passes** (`ab3_bb2_equivalence`,
   `_runerr`) compile every old-form corpus and bug-orig source with
   blitz2 and blitz2-ab3 (BB2 mode); model tests are not in them, so the
   move takes about 300 sources out (and the earlier moves already took
   the moved bug-orig tests out). The plan assumes the drivers the build
   writes (all model tests with a prev comparison, the earlier moves
   included) and the matrix's data sources join the equivalence passes in
   this change (task 5.3); answered 2026-11-24: "(a) In this change
   (Recommended)".
3. **If the measurement shows a loss** in a preset: stop before the removal
   and ask, with the numbers.

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `testing/expression-matrix`: "Each class has its test" and "Generated
  sources and their check" in the model form; a requirement on the form of
  the generated tests and on their run time against the old form.

## Impact

- `tools/lib/exprmatrix/expect.c`, `exprmatrix.h` (the writer);
  `tools/exprmatrix/cmd_expect.cpp`, `committed-broken.cmake`,
  `README.md`; `tools/tests/test_exprmatrix_expect.cpp` (and
  `test_covsel.cpp`'s example names).
- `tests/bug-orig/compiler/compiler-expr-*`, `tests/bug-orig/regression/compiler-expr-refused-*`
  (replaced), `tests/bug-orig/exprmatrix/` (new: the corpus's tests and its
  `tests.toml`), `tests/bug-orig/CMakeLists.txt` (the EXCLUDE of the
  `bug_orig_` registration, the registration `corpus_exprmatrix_`),
  `tests/corpus/CMakeLists.txt` (the loop and `NO_EXPECT_TARGETS` go;
  `expr-matrix-update` stays), `tests/corpus/tests-exprmatrix/*.bas` and
  `tests/corpus/expect/corpus_exprmatrix*` (removed; `matrix.txt`,
  `measured.tsv`, `left-out.txt` stay), `tests/ab3/CMakeLists.txt` (task
  5.3, by open question 2).
- Docs as above.
- Overlaps with open changes: `tests/README.md` and `TODO.md` are also
  edited by `ab3-mode-g5`, `blitz-coverage`, `coverage-ted-libs` (and
  `TODO.md` by `buffer-access-annotations`): other sections, no order.
  `expr-tree-5-operand-typing` runs `exprmatrix expect` and names the
  matrix's tests in its tasks 1.2, 2.1 and 3.2 (design decision 9; its
  task texts are adjusted by task 7.3). `compare-compound-left` task 2.3
  checks "the matrix outputs unchanged": holds for either form. Order: after
  `expr-tree-4b-lookaheads`, before `expr-tree-5-operand-typing` (the user's
  slot).
