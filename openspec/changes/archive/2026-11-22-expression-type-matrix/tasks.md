# Tasks

Before `quick-long-arithmetic` (#1178) starts (user decision 2026-11-22, "Order of the Quick work; matrix test"). Test form decided (design decision 4). The Decide question of design decision 2(a) is answered after task 1.2 and before task 1.3; the question classes of decision 7 before group 3. No compiler or library code changes.

## 1. Generator and measurement (measure first)

- [x] 1.1 `tools/exprmatrix` with `tools/lib/exprmatrix` (C, CLI11 per `docs/development.md` "Host programs", README, `-h`): the matrix description format, `generate` (sources with keys, value sets, the output forms of design decision 8, split at N cells), snitch tests in `tools/tests/` with broken descriptions (unknown kind, operator, context; duplicate key); verify: `ctest --preset host` and the same with `-DHOST_SANITIZE=ON`.
- [x] 1.2 Spike (report, no commit): one source of 200 cells, compiled and run with 1.95 and the port: code bytes per cell, compile and run time per session, the N that half fills the default Object Buffer; 20 cells alone against the same cells inside the full source (same values); whether AmiBlitz3 and blitz2-ab3 take the source as `.bb`; whether any two-operand cell reaches register depth 7 (it would be a `noncompile` cell after the #1185 fix, design decision 6). Report `~/.local/share/bb2-c-work/expr-matrix/spike-report.md` with the exact cell and source counts of options F, R, S of design decision 2; verify: the report gives the numbers the decision 2 question needs.
- [x] 1.3 Measure every cell of the chosen cut (after the answer to decision 2) with `c210`, `c195`, `portcd`, `clib` through `corpusrun pass --observe` and with AmiBlitz3 through a temporary `ab3_cli` list (not committed); the index cells also with runtime error checking on; outputs and the cell table in `~/.local/share/bb2-c-work/expr-matrix/`; verify: every cell has a value or `noncompile` for each compiler, and a repeated run of 10 sources gives the same outputs.

## 2. Expected values and questions

- [x] 2.1 The model (`tools/lib/exprmatrix/model.c`): the type so far, conversions and integer values by the original's rules and by the rules with the fixes (design decision 7, each rule with a comment naming its entry in `task-decisions.md`); snitch tests with the measured rows of `docs/blitz2/expression-types.md` as cases (both rule sets); verify: `ctest --preset host` with and without `-DHOST_SANITIZE=ON`.
- [x] 2.2 `classify`: reads the measured outputs and the model, writes the cell table (class per cell, design decision 6) and `expr-matrix-questions.md` (one question per class with three example cells, the four values and the options); snitch tests with a recorded small table (each class at least once, a broken output line); verify: `ctest --preset host`.
- [x] 2.3 Run `classify` on the measurement: the model's original-rules value equals the 1.95 value for every cell that compiles; every mismatch is resolved as a model error (fixed in 2.1) or reported as a new find with its D-021 class (`new`, `port`); verify: the report `~/.local/share/bb2-c-work/expr-matrix/classify-report.md` lists 0 unresolved mismatches, the counts per class, and the questions file.
- [x] 2.4 Questions to the user: the coordinator puts `expr-matrix-questions.md` into the open-questions file; the answers are recorded by the coordinator (D-001) and become rules of the model with the entry named (2.1); new finds become issues (D-008, D-021); verify: no cell of class `question` left for the cells to be committed, or the remaining ones named as left out.

## 3. Test files (after the answers to decision 2 and the question classes)

- [x] 3.1 `expect` and the target `expr-matrix-update`: write the sources and expected files into the source tree (corpus pass sources and `.out` files, `.orig.out`/`.fixed-N.out` pairs for fixed cells, bug-orig `compiler-expr-*.bas`/`.expect` with the header naming `BUG(orig) (#N)` and the cells); host test `exprmatrix_committed` and its broken-input test (a changed committed file); verify: `ctest --preset host`, `git diff` shows only generated files.
- [x] 3.2 The corpus pass `corpus_exprmatrix` (and `corpus_exprmatrix_runerr` for the index cells) in `tests/corpus/CMakeLists.txt`; verify: `ctest --preset all -R corpus_exprmatrix` passes, and `ctest --preset originals -R corpus_exprmatrix` passes against 2.10.
- [x] 3.3 Record the `.orig` files of the bug-orig tests: `cmake --preset amiga -DBUG_ORIG_RECORD_FILTER=compiler-expr-*` and `--target bug-orig-record`; verify: `ctest --preset bug-orig -R bug_orig_compiler_expr` gives REPRODUCED for every single-issue test (DIFFERS for none), and `ctest --preset originals -R bug_orig_record_compiler_expr` passes.
- [x] 3.4 Check each open test's expectation with a temporary patch where one exists (the branch of `quick-long-arithmetic`'s spike, if available), otherwise by the model only; verify: the report names which tests were checked FIXED with a patch.
- [x] 3.5 AmiBlitz3: `ab3_cli_case(... REF REF_EXPECT ...)` for the agreement and fixed sources, `REF_EXPECT` from AmiBlitz3's measured output; rows in `docs/ab3/variant.md` for the cells where AmiBlitz3 differs; verify: `ctest --preset oracle` and `ctest --preset all -R '^ab3_cli'` pass.
- [x] 3.6 Selection: `tools/testinfo/rules.txt` maps the description, the generated files and `tools/lib/exprmatrix` to the new tests; verify: the selection test of `testing/test-selection` with a changed description file.

## 4. Checks

- [x] 4.1 `ctest --preset host`; the targeted tests of 3.2, 3.3, 3.5; a clean rebuild without warnings; the prev comparison shows no changed result (rule 3: no `PREV_INTENDED`); verify: the commands pass, the prev report lists no difference.

## 5. Documents

- [x] 5.1 `tests/README.md`: the matrix (the pass, the bug-orig tests, the classes, DIFFERS between two fixes, the update target, the questions file); `tools/exprmatrix/README.md`; `docs/blitz2/expression-types.md`: a pointer to the matrix as the measured source of its tables; verify: the coordinator's review, links resolve.

## 6. Commit and follow-up

- [x] 6.1 Commits by topic: the tool with its tests; the generated tests with their `.orig`/expected files and the CMake lists; the documents. The coordinator runs `all`, `bug-orig` and `originals` once after the merge (D-010); verify: the coordinator's runs.
- [x] 6.2 Update `quick-long-arithmetic` with `openspec-update-change` (its plan, merged in 0a94a890f, builds on the matrix and names it "once it exists"): the change name `expression-type-matrix` and the test form in its proposal, design and tasks 1.2, 3.1, 3.4 and 5.1: the matrix's #1178 tests (`; @fixed #1178`), its corpus files (pairs `NAME.orig.out` / `NAME.fixed-1178.out`) and the regeneration with the fixed rules; verify: `openspec validate quick-long-arithmetic --strict`.
