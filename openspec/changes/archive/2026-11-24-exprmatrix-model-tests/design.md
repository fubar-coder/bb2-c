# Design

## Context

The writer of the matrix's tests is `em_expect` in `tools/lib/exprmatrix/expect.c`
(`emit_corpus`, `emit_open`, `emit_refused`, `emit_ab3`, `left_out`); the
model and the classes (`model.c`, `classify.c`) give each cell, per mode,
the original's value, the expected value and the port's value with both
library sets. Today (counted on main, 2026-11-24; `docs/testing-old-form.md`
section 3 still says 46/36/123):

| Group | Files | Tests | Run |
|---|---|---|---|
| bug-orig, open/fixed cells | `tests/bug-orig/compiler/compiler-expr-ISSUES-CTX-SET[-u\|-c]-NN.{bas,expect,orig}` | 52, all `; @fixed-orig #N` | one session each, port; `bug_orig_record_*` against `.orig` on the originals side |
| bug-orig, cells a fix refuses | `compiler-expr-{1179,1188}-refused-OP-L-R[-c]` | 36 (7 + 29) | compile only |
| regression, refused by all | `tests/bug-orig/regression/compiler-expr-refused-OP-L-R[-c]` | 123 (8 operators) | compile only |
| corpus | `tests/corpus/tests-exprmatrix/CTX-SET[-u\|-c]-TAG-NN.bas`, `tests/corpus/expect/corpus_exprmatrix[_runerr]/` | 95 sources, each a pass `corpus_exprmatrix[_runerr]_NAME` and `_originals` | `all`: port with the C libraries (+ prev); `originals`: 2.10/CD, port/CD, 2.10/C libraries, one session each |

The model's bundles (`tj_bundle_case`) take a case whose test has one cell
on the side port, no `[data]`, no `isolate`/`serial`/`editor-run`/`hang-after`,
a plain session key (the environment's limit, 30 or 60) and no own
whole-program assertion (`rc`, `output`, `compile`, ...; `tj_case_alone`).
Measured (analysis-bundling, 2026-11-23): a model session costs at most
0.2 s more than an old one (1.61 s against 1.41 s median); 38 generated
sources in one session took 3.73 s against about 1.0 s each alone.

The matrix's own measurement (`~/.local/share/bb2-c-work/expr-matrix/measurement-report.md`)
found two things that bear on shared sessions: (1) Byte-destination `MOD`
cells of 1.95 and 2.10 ended with "Software Failure" when run after other
programs in one session, alone they ran; (2) unchecked index reads beyond
the array disturbed later programs. The committed unchecked index cells are
only those every compiler reads inside the array (design decision 8 of
`expression-type-matrix`), so (2) should not occur, but it is not proven.

## Goals / Non-Goals

**Goals:** the generator writes the model form for every group; the port
side bundled where the bundle rule allows; the old form, its expected files
and the paths only it uses removed; the run time not worse than the old
form (proposal, the user's condition); `expr-tree-5` can use the matrix as
planned.

**Non-Goals:** a new runner or test-library feature; bundling the
originals side or data-source tests; any change of an expected value.

## Decisions

Every decision below is a choice of the planning agent (D-011 (b)), rated
with the catalog; none is the user's. Those resting on a recorded answer
name it.

1. **Where the corpus's tests live: `tests/bug-orig/exprmatrix/`**, with
   a `tests.toml` (`[matrix] compiler = ["port", "orig210"]`, `libs =
   ["cd", "clib"]`: the four configurations of the old passes: port/clib in
   `all`, 2.10/CD, port/CD and 2.10/clib in `originals`), registered by
   `basic_test_register(corpus_exprmatrix_ ... "exprmatrix/*.test.bas" PREV
   "^exprmatrix/")` and left out of the `bug_orig_` registration (EXCLUDE).
   Why: `fix-scope` projects every area of `tests/bug-orig` with `--bug-orig`
   (model tests included), so the corpus's checks stay under the fix-scope
   check that `tests/corpus/expect` gave them; `regression/` is the
   precedent of non-bug-orig tests in that tree; the prefix keeps
   `-R corpus_exprmatrix`. Alternatives: `tests/corpus/tests-exprmatrix/`
   (the walker of `bugorigcmp` expects `ROOT/AREA/`; fix-scope would need a
   new mode, a tool change with tests) or `tests/blitz2/` (outside fix-scope).
   `matrix.txt`, `measured.tsv` and `left-out.txt` stay in
   `tests/corpus/tests-exprmatrix/` (`test_exprtype` and the rules read them
   there).
   `Point: the location of the corpus's model tests — L2 Possible / I2 Medium / E2 Reasoned → Plan`

2. **A cell in the model form.** One case per source (name: a word that is
   no token of any library set, checked by the registration; `cells` if
   free), the declarations (variables, `Dim`, the `For` that fills the
   arrays) inside the case, the procedures of the parameter contexts
   outside it. Per context:

   | Context | Old line | Model line |
   |---|---|---|
   | `db` `dw` `dl` | `rl=EXPR : NPrint "KEY ",rl` | `rl=EXPR : !CheckEq{rl,V,"KEY"}` |
   | `dq` `df` | `rq=EXPR : NPrint "KEY ",Hex$(Peek.l(&rq))` | `rq=EXPR : !CheckEq{Peek.l(&rq),$BITS,"KEY"}` |
   | `pl` `pw` | `Print "KEY " : pl{EXPR}` (prints its argument) | `pl{EXPR} : !CheckEq{gl,V,"KEY"}` (the statement stores its argument in a `SHARED` global) |
   | `pq` `pf` | as above, raw bits | `pq{EXPR} : !CheckEq{Peek.l(&gq),$BITS,"KEY"}` |
   | `if` | `If EXPR Then NPrint "KEY T" Else NPrint "KEY F"` | `If EXPR Then t=1 Else t=0` `: !CheckEq{t,1\|0,"KEY"}` |
   | `i1` `i2` unchecked | `NPrint "KEY ",ia(EXPR)` | `vv=ia(EXPR) : !CheckEq{vv,V,"KEY"}` |
   | `i1` `i2` checked | `SetErr : NPrint "KEY !runtime" : Goto mxN : End SetErr` / `NPrint "KEY ",ia(EXPR)` / `mxN:` | `vv=0 : SetErr : vv=-1 : Goto mxN : End SetErr` / `vv=ia(EXPR)` / `mxN:` / `!CheckEq{vv,V\|-1,"KEY"}`; `ClrErr` at the end of the case |
   | `print` | `NPrint "KEY ",EXPR` | unchanged; judged by `output` (decision 3) |

   Raw bits as a Long with a hex constant: exact, as the old `Hex$`, and
   no Quick or Float constant of the expected value goes through the
   compiler's conversions that the matrix tests (`!CheckQEq` would). An
   element is `i*16+1`, never 0 or -1, so `-1` names the runtime error and
   an unexpected one leaves `0`. The check's name is the cell's key
   verbatim (`check.cells.dl-mul-q-l-A`; the query takes any text after the
   namespace, `tc_query_parse`), so a failure names the same key as
   `left-out.txt`, `measured.tsv` and the classify tables.
   `Point: a model line gives the same value as the old line for every committed cell (the index read into a Long, the parameter stored instead of printed, the check call between two cells) — L2 Possible / I2 Medium / E2 Reasoned → Plan` (task 1.1 compiles and runs one source per context on the port and both originals before the generator is changed; a cell whose value changes is reported, not adjusted).

3. **The `Print` context by `output`.** A `Print` cell's context is the
   statement itself; no check can take it. The case keeps the `NPrint`
   lines and the test asserts `output == "..."` (the old `NAME.out` without
   `rc=0`), its deviation the original's output. `output` is a
   whole-program assertion, so these tests are not bundled (`tj_case_alone`):
   8 corpus sources and 7 bug-orig tests run alone, as today. Alternative:
   a test-library feature that captures what `Print` writes (out of scope;
   a feature of its own).
   `Point: Print sources stay unbundled — L3 Likely / I1 Low / E3 Proven (tj_case_alone, testjudge.c) → Monitor`

4. **Deviations and `fixed-orig`.** The generator writes, for every cell
   whose original value differs from the expected one, `"check.CASE.KEY" =
   "== fail"` and `"check.CASE.KEY.actual" = "== V"` (the signed decimal of
   the Long or of the raw bits) in `[deviations."SELECTOR"]`: the selector
   of the configurations that give the original's value: a compiler fix
   `compiler=orig195` (bug-orig area) or `compiler=orig210` (corpus); a
   library fix (#199, #234) `libs=cd`; a cell where both matter the selector
   of each configuration (`compiler=port,libs=cd`, ...), the most specific
   winning (`tests/README.md`, "One assertion per query"). The original's
   values come from the model's original rules and `measured.tsv` (1.95 and
   2.10 agree on every committed cell: a cell where they differ is a
   question, Q-F, and in no test); the `ID.orig` recordings go. A source of
   a fixed issue gets `fixed-orig = N` (the last of its issues for
   `ISSUES` of two, as the header today); a source of open cells none, so
   it comes out REPRODUCED (rule 4; the precedent "Generator writes new
   name" covered the old form only). The corpus's 2.10-with-C-libraries
   cell was never measured; its value is the model's (a compiler fix: the
   original's, a library fix: the fixed one), as the old pass accepted
   "either".
   `Point: a generated deviation does not match what the original prints (the .orig was a recording, the deviation is a prediction; 2.10 with the C libraries is unmeasured) — L2 Possible / I2 Medium / E2 Reasoned → Plan` (the first `originals` run of task 3.3/4.3 checks every cell; a mismatch is fixed in the generator or reported).

5. **The refused cells as data sources**, grouped: the 123 regression
   cells into one test per operator (`regression/compiler-expr-refused-OP.test.bas`
   with `ID.test/L-R[-c].bas`, 8 tests), the 36 bug-orig cells into one test
   per issue set (`compiler-expr-1179-refused`, `compiler-expr-1188-refused`).
   Each data source is the old source's text, so its error keeps its line;
   `"data.L-R.bas" = "== \"error N MESSAGE\""` (the `-c` ones with the
   action's `runtime-errors`); for the bug-orig tests the deviation of
   `compiler=orig195` is `== ok` per data source and `fixed-orig = N`. The
   case is trivial (one check), so the driver exists. Why grouped: a test
   with data sources is not bundled (`conf->ndata`), so 159 tests of one
   data source would cost 159 sessions and break the user's condition;
   grouped they are 10 tests. Alternative: one test for all 123 (one
   failure would hide behind one ctest name; per operator keeps the
   failures apart).
   `Point: the grouping of the 159 refused cells (10 tests, the IDs change) — L2 Possible / I2 Medium / E2 Reasoned → Plan`

6. **Bundling and isolation.** The port side is bundled by the existing
   rule; nothing new. The unchecked index sources (`-u`) get `isolate =
   "on"` first (finding (2), the analyst's advice "start ... without the
   unchecked index sources"), as alone today; task 5.2 tries them bundled
   and drops `isolate` only if two full runs give the same verdicts. The
   originals cells of a test share their session (model default); a test
   whose originals cell fails only in a shared session (finding (1), the
   Byte `MOD` cells) gets `isolate` for those cells by the generator
   (`isolate = ["compiler=orig210"]`), decided from the first two
   `originals` runs.
   `Point: a bundled or shared session disturbs a matrix program and a verdict changes — L2 Possible / I2 Medium / E3 Proven (measurement-report.md findings 1, 2) → Plan`

7. **Size of a source.** Today a group is one source (`cells-per-source`
   2000, up to 3,300 lines). In a bundle a chunk of 16 members shares the
   session's time limit, and every cell becomes a check call (more code to
   compile, one result entry per check). Task 1.1 measures the compile and
   run time of the largest group in the model form on the port and on both
   originals; the description's `cells-per-source` is lowered (more
   sources, still bundled) if a chunk would not fit the plain key's limit,
   or a source would not compile with an original.
   `Point: a large model source does not compile with 1.95/2.10 or a chunk of 16 overruns the session limit — L2 Possible / I2 Medium / E1 Assumption → Plan` (E1: task 1.1 is the measurement that raises it before the writer is built).

8. **The measurement** (the user's condition; precedent: "Group 4 ...
   the old form \"for now\"" and "Bundling answers", quote (2): bundling may
   mitigate, the criterion is not slower in all): two full runs of `ctest
   --preset all` and one of `--preset originals` on main (the old form,
   after `expr-tree-4b`) and the same on the branch with all groups moved,
   on the same machine with nothing else running; from the JUnit files
   (`testinfo stats`) the wall time of each run and the sum of session
   times of the matrix's tests (old: `corpus_exprmatrix*`,
   `bug_orig_compiler_expr_*`, `regression_compiler_expr_refused_*`,
   `bug_orig_record_compiler_expr_*`; new: their successors and the bundle
   tests' share by member). Not slower in each preset: go on with the
   removal; slower in one: stop and ask (open question 3). The report goes
   to `~/.local/share/bb2-c-work/exprmatrix-model-tests/measurement.md`.
   The old and new form cannot be measured in one tree, so the baseline is
   taken first (task 1.2).

9. **`expr-tree-5` after the move.** Its task 1.1 (rule sets in `model.c`,
   snitch tests) and 2.2 (the type so far from `model.c`, `test_exprtype`)
   do not touch the writer and work unchanged. Task 1.2 ("the named-cell
   list generated (`exprmatrix expect` with the new rule)") works
   unchanged: the list is the cells whose classes change; `git diff` then
   shows them as changed checks and deviations in `ID.test.toml`/`.test.bas`.
   Task 2.1 ("the named cells as bug-orig/fixed tests in the matrix's form
   (REPRODUCED on main)"): the generator writes them as model tests of open
   cells (no `fixed-orig`, deviations of `compiler=orig195`), REPRODUCED;
   its verify regex `exprmatrix|operand-order` matches no ctest name of
   them (`bug_orig_compiler_expr_*`). Task 3.2 (FIXED, fix-scope): the fix
   commit adds `fixed-orig = N`, the only fix-scope case of the issue. Task
   7.3 of this change rewrites the regex and the words "expected files" /
   "matrix pass" of `expr-tree-5`'s proposal, design and tasks with
   `openspec-update-change`.
   `Point: expr-tree-5's task texts name the old form — L4 Certain / I2 Medium / E3 Proven (its tasks.md 2.1, design.md "fails the matrix pass") → Plan`

10. **What goes** (only where nothing else uses it, checked by grep in
    task 6.1): the `.bas`/`.expect`/`.orig` of the groups;
    `tests/corpus/tests-exprmatrix/*.bas`; `tests/corpus/expect/corpus_exprmatrix`
    and `corpus_exprmatrix_runerr`; in `tests/corpus/CMakeLists.txt` the
    `tests-exprmatrix` entry of the tokenising loop and the pass loop, the
    `corpus_pass` option `NO_EXPECT_TARGETS` (its only user); the old-form
    writer in `expect.c`. Stays: `corpus_pass`'s `EXPECT` if another pass
    uses it, the corpus's pair logic (`corpusexpect`, used by the notes
    rows), `bug-orig-record` and `check-fixed-orig.cmake` (TED and the other
    old-form tests), `expr-matrix-update`, `exprmatrix_committed`.

11. **Commits** (one topic each; every commit leaves `exprmatrix_committed`
    green): (a) the refused cells as data sources (writer, host tests,
    regenerated files, old files of that group); (b) the bug-orig run tests;
    (c) the corpus (new area, registration, old passes and expected files
    out); (d) the equivalence passes (by open question 2); (e) docs and
    rules. Each generator commit names "the test programs' executables
    change (model drivers); no compiler, library or TED result".

## Risks / Trade-offs

- [Coverage of blitz2-ab3's BB2 mode drops] the equivalence passes compile
  only old-form sources → open question 2; the plan adds the drivers and
  data sources of the moved tests (task 5.3).
- [A served bundle verdict hides a disturbed program] → `cr_cell_clean`
  stores only clean ends; decision 6 keeps the doubtful sources isolated.
- [The originals side gets slower] (model session overhead, unbundled) →
  the data-source grouping and the shared originals session (three cells in
  one session instead of three sessions for the corpus) should outweigh it;
  measured (decision 8).
- [The `fixed-orig` fix-scope case of a corpus source] a later fix that
  moves agree cells into a fix source shows them as removed checks of the
  agree test (as today a changed `NAME.out` did) → reviewed and named in
  the fix commit (tools/results, "fix-scope").
  `Point: fix-scope reports matrix cells a fix moves between sources — L3 Likely / I1 Low / E2 Reasoned → Monitor`
- [The `orig210` cells need the fixture `rwe_refbuild`]
  (`--cell-fixture compiler=orig210|rwe_refbuild`) → selecting only the
  matrix's `_originals` tests also builds the RWE reference; costs time in
  a selective run only.
  `Point: orig210 cells pull the rwe_refbuild fixture — L4 Certain / I1 Low / E3 Proven (cmake/basic-test-model.cmake) → Monitor`

## Migration Plan

The groups move one by one (decision 11); each commit regenerates with
`expr-matrix-update` and passes `exprmatrix_committed`, `ctest --preset
host`, the moved tests on both sides and the prev comparison. Rollback of a
group is a revert of its commit (the generator and its files go together).
The baseline measurement (task 1.2) comes before the first move.

## Findings (implementation, 2026-11-24)

- **Ids in lower case.** The registration of the model refuses an id with
  an upper-case letter (`tools/lib/testreg/testreg.c`, `id_ok`), and the
  value sets are upper case (`A`, `B`, `I`, `J`). The tests keep their
  names in lower case (`compiler-expr-1178-dl-a-01`, `dl-a-agree-01`); the
  check names keep the cell keys (`dl-mul-q-l-A`). Q1's "same IDs" holds
  up to the case.
- **Spike (task 1.1)**: nine sources converted by hand, every cell the old
  value on the port and in the four cells with original material;
  `cells-per-source` stays 2000 (the largest source counts about 3.3 s on
  the port). Report:
  `~/.local/share/bb2-c-work/exprmatrix-model-tests/spike.md`.
- **Originals mismatch (task 3.3), fixed in the generator**: the first
  `originals` run failed the 12 bug-orig tests of #1189 in the integer
  and index contexts: the port with the CD's libraries gives the
  original's value there, as #1189's fix is in the C library
  (mathtranslib's power), while the model applied the fix with both
  library sets. `EM_RULES_LIBFIX` (`exprmatrix.h`): the port with the CD's
  libraries keeps the original's rule of such a fix; a power the model
  does not compute takes the original's value where its operands are the
  same. The cells of #1189 with a second issue then give a third value in
  that configuration (`compiler=port,libs=cd` deviations). After the fix
  every moved test passes on both sides; two more `originals` runs gave
  the same verdicts (task 4.3: no corpus mismatch, the cell of 2.10 with
  the C libraries included).
- **No shared-session failure** (decision 6): the side originals runs the
  cells of a test in one session; no cell failed there.
- **The unchecked index sources bundled (task 5.2)**: with `isolate` off
  for the 15 `-u` sources two full `all` runs passed every emulator test,
  the 15 served from their bundles (the same verdicts as isolated); the
  generator writes no `isolate`. Matrix session sum 848.4 s and 815.0 s
  (against 869.4 s and 884.6 s isolated).
- **Deviation tables.** A check's deviation goes to the table of the
  original compiler when only it differs, to `libs=cd` when only the CD's
  libraries differ, else to the table of each configuration that differs.
  An open test whose port with the CD's libraries still gives the
  original's value has its deviations under `libs=cd`; with the fix that
  configuration gives the fixed value, so the fix commit changes the
  tables as well as adding `fixed-orig` (expr-tree-5's task 4.2 says so).
- **Equivalence (task 5.3)**: 638 sources per pass. Four drivers
  (`compiler-expr-1178-dl-b-01`, `-i1-j-c-01`, `-i2-j-c-01`, `-pl-b-01`)
  gave different executables with blitz2 and blitz2-ab3: a linked library
  routine behind the division routines keeps two JSRs with the
  compiler's addresses (not relocated). The cause is a port regression,
  #1215 (after the "Division by Zero" of the `compiler-constdiv-*`
  sources earlier in the session the compiler writes no operator code and
  drops modlib's relocations); they are left out of the comparison until
  #1215 is fixed.
- **Commits.** The writer of the three groups is one generator, and its
  output must equal the committed files in every commit; the groups
  (decision 11's commits (a) to (c)) are one commit, the equivalence
  passes (d) and the documents (e) follow.
- **AmiBlitz3 cases** byte-identical; their header still names
  `tests/corpus/tests-exprmatrix/NAME.bas`, the old place of the source.
