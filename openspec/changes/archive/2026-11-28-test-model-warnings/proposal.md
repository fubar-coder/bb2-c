## Why

The compiler has named warnings (spec `compiler/warnings`), and the change
`type-system-rework` adds seven type warnings in both modes (its task
3.5). The BASIC test model cannot judge a warning: the editor path has
only the WARNING lines inside `compile.log` (as
`tests/blitz2/warn-datafrac-limit` asserts them, with driver lines and one
long `contains`), the command line has neither a query of the warnings
nor of the compiler's return code. So the first warning tests of AB3 mode
(the `datafrac*` cases of commit c27b14450) went into `tests/ab3/cli` as
`.expect` cases. The user decided (tasks/decisions.md, "2026-11-28
Compiler warnings in the test model", active): "Eigene OpenSpec-Change
(Recommended)", with the rule "the test model gets a query for the
warnings of a compile (line, name, text; the return code on the command
line) in an OpenSpec change of its own (testing/test-model delta),
planned first and shown to the user; then the datafrac cases move from
tests/ab3/cli into the model and the type-warning tests of
type-system-rework are written there."

## What Changes

- New queries of the BASIC test model (`testing/basic-test-runner`):
  - `compile.warnings`: the warnings of the driver's compile, one line per
    warning in the order the compiler gave them, each `PLACE NAME TEXT`
    (PLACE the line of the test source, `driver:N` for a driver line
    outside it, another file's place as the compiler names it, `-` without
    a place; NAME the warning's name, `-` where the compiler names none;
    TEXT the message). The editor's `NAME: K more` is the entry
    `- NAME K more`. Predicates: `==`/`!=` string, `contains`, `matches`,
    `exists`, `count CMP integer`. No default assertion.
  - `compile.warnings.K`: the K-th entry (from 1), so that a list is
    asserted one warning per TOML line; a K beyond the list is not
    captured.
  - `compile.rc`: the return code of a command-line compile (`compiler =
    "ab3"` or `"amiblitz3"`), `CMP integer`; refused in a cell of the
    editor path.
- Both paths: the editor path (blitz2 through tedstub, BB2 mode: the
  WARNING lines of job `j000`'s block, `Warning: line N: TEXT [NAME]` and
  `Warning: NAME: K more`) and the command line (blitz2-ab3 `-s ... -e
  ...`, AB3 and BB2 mode: `PLACE: warning: MESSAGE [NAME]` lines of the
  console). The original compilers' cells give the empty list.
- Refusals at registration, each with a test of deliberately broken input:
  `compile.rc` in a cell that does not compile from the command line;
  `compile.warnings` and `compile.warnings.K` in a cell of `amiblitz3`;
  `K` not a decimal from 1; the jobline IDs `warnings` and `rc` (they
  would be the queries `compile.warnings`, `compile.rc`). At judge time a
  WARNING line or console warning line the parser cannot read, or a name
  that is not in `shared/ab3job/warnlist.h`, makes the cell
  cannot-be-judged (ERROR), naming the line.
- A query of the warnings or of `compile.rc` is an observation of the
  whole program: such a case runs on its own and shares no bundle session.
- `testing/test-model`: compiler warnings are judged one by one (a
  missing, an added and a reordered warning each fail).
- `testing/ab3-tests`: a case whose subject is a compile's warnings and
  its return code with them is a test of the model; the cases that need
  other arguments (`-w`, `-werr`) or `SAME` stay in the command-line form.
- The `datafrac*` cases of `tests/ab3/cli` move into `tests/ab3/model/`
  as far as the model expresses them (design D7): `datafrac`,
  `datafrac_bb2`, `datafrac_many`, `datafrac_inc` (if the include can be
  staged next to the driver); `datafrac_r`, `datafrac_off` (`-w`, `SAME`)
  and `datafrac_werr` (`-werr`) stay. Each moved case keeps its
  assertions: every warning line and `rc=` of its `[compile]` section,
  every value of its `[run]` section.
- `tests/blitz2/warn-datafrac-limit` asserts with the query instead of
  `compile.log` (source lines instead of driver lines).
- `tests/README.md`: the grammar, the query table, "Tests of AB3 mode",
  the "Not yet moved" note (updated, not dropped); the Purpose of
  `openspec/specs/testing/test-model/spec.md` likewise.
- A note in `openspec/changes/type-system-rework/tasks.md`, task 3.5: its
  warning tests are written with these queries (the editor path in
  `tests/blitz2`, the console in `tests/ab3/model`); its check "the
  executables unchanged with the warnings disabled" stays an `ab3_cli`
  case with `SAME`.

## Not in scope

- A run setting for the warning sets (`-w`, `-werr`, tedstub's
  `warnings=`) and free compiler arguments: the cases that need them stay
  in `ab3_cli` (design D6).
- `SAME`, `HUNKS`, other trees, fixed dates: unchanged reasons of the
  "Not yet moved" note.
- Warnings of a data source or a jobline job as a query of their own
  (their WARNING lines stay in `compile.log.JOB`).
- Moving the `wclash*` cases (resclash, residents found by name) and the
  compile-error cases of `ab3_cli`; the `procend` cases of ab3-mode-g5
  (open question Q1).
- The warnings of AmiBlitz3 (the oracle side): refused, not parsed.
- Any change of blitz2, blitz2-ab3, tedstub or the compiler's messages.

## Tests

- Host: the parsers of both forms (`test_caserun.c`: editor WARNING lines,
  console lines, the `more` line, a place in another file, a driver line
  outside the source, broken lines and unknown names); the queries and
  refusals (`test_testassert.cpp`, `test_testconf.cpp`); the judge
  (`test_testjudge.cpp`: a missing, an added, a reordered warning, `count`,
  `compile.warnings.K` beyond the list, `compile.rc`); `testconf_readme`
  for the README examples.
- Emulator: the moved `ab3_warn-datafrac*` tests (side port), the
  rewritten `blitz2_warn-datafrac-limit` (port and orig195), the
  remaining `ab3_cli` datafrac cases; one deliberately broken run per
  path (an expected warning on a wrong line) that must fail naming the
  query.
- `ctest --preset host`, also with `-DHOST_SANITIZE=ON`.

## Results that change

None (CLAUDE.md rule 3). No compiler, TED, library or executable changes;
the runner and the judge are host tools. The moved cases leave
`ab3_cli_prev` with their `ab3_cli` cases; the new model tests get the
model's prev comparison of their drivers. Every commit is checked with the
prev build and without `PREV_INTENDED`.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `testing/test-model`: a requirement that compiler warnings are judged
  one by one.
- `testing/basic-test-runner`: the queries `compile.warnings`,
  `compile.warnings.K`, `compile.rc`; their refusals; the warnings of both
  paths; the jobline IDs `warnings` and `rc`; the list of queries and the
  observations of the whole program.
- `testing/ab3-tests`: which warning cases keep the command-line form; a
  moved case keeps its `[compile]` lines.

## Impact

- Code: `tests/corpus/runner/caserun.c`/`.h` (the parsers, the
  observations), `tools/lib/testconf/testassert.c`, `testconf.c`,
  `testmatrix.c`, `testconf.h` (queries, refusals, jobline IDs),
  `tools/lib/testjudge/testjudge.c`/`.h` (judging), their host tests in
  `tools/tests/` and `tests/corpus/runner/`.
- Tests: `tests/ab3/model/warn-datafrac*.test.{bas,toml}` new;
  `tests/ab3/CMakeLists.txt` and `tests/ab3/cli/datafrac*.{ab3,bb,expect}`
  of the moved cases removed; `tests/blitz2/warn-datafrac-limit.test.toml`.
- Docs: `tests/README.md`; the Purpose of
  `openspec/specs/testing/test-model/spec.md`; the comment above the
  `ab3_cli_case` list in `tests/ab3/CMakeLists.txt`.
- Other changes: `type-system-rework` task 3.5 (a note); `ab3-mode-g5`
  task 4.3 (Q1); shared files with open changes: design D9.

## Open questions

- Q1 (for the user): the `procend` warning case of ab3-mode-g5 task 4.3:
  in the model with the query, or in `ab3_cli` as that task says (design
  D9).
