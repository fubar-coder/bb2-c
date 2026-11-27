# Design: ab3-test-model

## Context

See proposal.md, "Why". Today (main 0f3eefe62):

- `tests/ab3/CMakeLists.txt` lists 225 `ab3_cli_case(...)` lines plus the 54
  generated ones of `cli/exprmatrix/cases.cmake`; `ab3_cli_blocks` cuts them
  into blocks of `AB3_CLI_BLOCK` (70) cases, `ab3_cli_N` (blitz2-ab3, label
  `emulator`) and `ab3_cli_ref_N` (AmiBlitz3, the cases marked `REF`, label
  `oracle`), each block one `tests/ab3/cli.cmake` session; `ab3_cli_prev_N`
  compares each block's executables with the prev build.
- The generated case lists of the build (`build/amiga/tests/ab3/ab3_cli_*.cases`)
  hold 280 exe cases and 236 ref cases. Sorted by what the expect file
  observes (a one-off count of this planning, method as in the spike):
  132 run output without arguments, 59 run output with `-r` (54 of the
  expression matrix), 67 compile errors (`rc` not 0), 2 console output,
  2 other arguments (`-w`/`-werr`), 5 fixed date, 1 norun, 8 `SAME`/`HUNKS`,
  4 another tree. The recorded "~131 run-output cases" are the 132.
- The BASIC test model runs a test through `corpusrun case`: the driver
  (`shared/basictest/macros.bas`, `TLBegin "CORPUS:out/..."`, the
  preprocessed source) is tokenised and compiled through tedstub; the
  `compiler` setting knows `port`, `orig195`, `orig210`
  (`tools/lib/testconf/testconf.c:20`).
- Spike results (E3, `ab3-testmodel-spike/report.md`): testlib.obj loads
  and runs under blitz2-ab3 in AB3 mode and under AmiBlitz3's CLI compiler
  (no other library needed, number 81 free, macro names pass AB3's token
  check); 20 bundled cases give identical result files on both; per case
  ~15 ms bundled vs ~90 ms in a block (blitz2-ab3), ~25 ms vs ~210 ms
  (AmiBlitz3); one session's fixed cost ~1.0 s / ~2.3 s. No `Defaultdbug`
  on the AmiBlitz3 side. Follow-up (`followup-runprogram.md`): AmiBlitz3
  removes the `Blitz3:` assign on exit; it hung once silently in ~45
  sessions (#976 kind).

## Goals / Non-Goals

**Goals:** the run-output cases of D10 as tests of the model on both
compilers, the same values checked; the infrastructure for `ab3-mode-g5`
to add its run-output cases in the new form; testpp reads `@@`.

**Non-Goals:** `!RunProgram` cases sharing a run; a `compile.output`/
`compile.rc` query of the console text and the return code (the
compile-error and warning cases stay `ab3_cli`); `exe.same`, `exe.hunks`;
the trees `Blitz3-nolib`, `-nobase`, `-userlib`, `none`; datestamps; a
change of any compiler, library or TED source.

## Decisions

Each decision is the planning agent's Plan or Monitor choice (D-011 (b))
unless it says "user"; the rating line follows the catalog.

### D1. The moved cases run through the model's runner, not through cli.cmake

The cells of `ab3` and `amiblitz3` are cells of `corpusrun case` like any
other: registration (`testreg`), driver (`testpp`), judge (`testjudge`),
bundles, prev comparison, the measured COST and the reruns come with it.
`cli.cmake` stays for the cases that keep the command-line form.

Alternative: feed `--write-driver` drivers to `cli.cmake` and judge its
result files (the spike's `run.cmake`). Rejected: a second runner of the
model, without bundles, reruns or prev comparison; it would keep the
technical reason of the "not yet moved" note.

`Point: the runner integration instead of cli.cmake — L1 Unlikely / I2 Medium / E3 Proven (spike report (a), (b): the driver runs unchanged) → Monitor`

### D2. Compiler values `ab3` and `amiblitz3`; the source mode

`compiler = "ab3"`: blitz2-ab3 of the build, started in the session as
`blitz2-ab3 -s DRIVER -e EXE` (plus `-r` when `runtime-errors = "off"`),
with `blitz2:` as for `ab3_cli` and the `Blitz3:` tree of D3.
`compiler = "amiblitz3"`: `Blitz3:Amiblitz3 -s DRIVER -e EXE` (plus `-r`
as above), the same commands `cli.cmake` uses. Neither needs the CD; the
AmiBlitz3 files are the fetched `archive/amiblitz3/github-58a11658`; a
cell without them is skipped (77) as today.

`source-mode = "ab3"` (default) writes the driver `ID.ab3`;
`source-mode = "bb2"` writes `ID.bb` (blitz2-ab3's BB2 mode from the
command line, the seven `*_bb2` cases); `bb2` with `amiblitz3` is refused
naming file and line. The setting is a run setting (it can be an axis).
Its name follows the spec `ab3-compat/source-mode`.

Alternative: a third compiler value `ab3bb2`. Rejected: the compiler is
the same program; the mode is the source's.

`Point: two compiler values and source-mode — L2 Possible / I2 Medium / E2 Reasoned → Plan`

### D3. The Blitz3: tree is staged by the runner, the default tree only

The runner stages `Blitz3:` as `cli.cmake` builds the tree `Blitz3` (the
four BigLibs, `Residents/all.res`, `UserLibs/` with testlib.obj, an empty
`Libs/`, `Sourcecodes/Includes`; for `amiblitz3` the compiler and
`ab3pft.library`), in C in the runner (a function the pass of
`ab3-mode-g5` task 1.2 can call). Before every `amiblitz3` compile of a
session the runner assigns `Blitz3:` again (AmiBlitz3 removes it on exit).
`amiblitz3` sessions run without the JIT (#976), as `cli.cmake`'s ref
configuration. The result volume is `CORPUS:` as in every cell
(`TLBegin "CORPUS:out/..."`), not `TEST:`.

`Point: Blitz3: staging in the runner, CORPUS: volume — L2 Possible / I2 Medium / E3 Proven (spike: testlib in Blitz3:BlitzLibs/UserLibs, follow-up pitfall (1)) → Plan`

### D4. The `compile` query of a command-line compile

`compile` of an `ab3`/`amiblitz3` cell is `ok` if the executable exists,
else `error LINE MESSAGE` with the line mapped to `ID.test.bas` (as the
runner maps tedstub's error line today) from blitz2-ab3's
`FILE:LINE: MESSAGE`; for AmiBlitz3, which returns 0 on compile errors and
writes its own text, `error ? TEXT` with the first error line of its
output. `compile.log` is the compiler's console output. A case whose
AmiBlitz3 compile fails (`.ref.expect` "no executable") is a test file of
its own (D7), so the other cases still run on AmiBlitz3.

`Point: compile query of a CLI compile — L2 Possible / I2 Medium / E2 Reasoned → Plan`

### D5. The side `oracle`

The cells of `compiler = "amiblitz3"` form a third side: tests
`PREFIX_ID__CASE_oracle`, label `oracle` (not `emulator`), so `emulator`
and `all` leave them out and `ctest --preset oracle` runs them, as for
`ab3_cli_ref_N` today. A cell needs original material (side `originals`)
only as before; `amiblitz3` is never on the side `port` or `originals`.

`Point: a third side with the existing preset — L1 Unlikely / I2 Medium / E3 Proven (CMakePresets.json "oracle": label oracle) → Monitor`

### D6. A difference of AmiBlitz3 is a declared deviation with its variant.md row

A test declares what AmiBlitz3 does instead in
`[deviations."compiler=amiblitz3"]` of its own file, with the required
key `variant = "ROW"`: the name in the first column of the row of
`docs/ab3/variant.md` ("Differences to AmiBlitz3's compiler" or the G
rows) that records the difference. Such a deviation needs no
`fixed-orig`/`fixed`/`extension` and never gives the label `bug-orig`.
The registration refuses a deviation of `amiblitz3` cells without
`variant`, `variant` anywhere else, and `variant` in a `tests.toml`,
naming file and line; a task checks that every `variant` value occurs in
`variant.md` (a build check with a broken-input test, rule "Every check
has a test with deliberately broken input").

Why not an issue: the recorded decisions give these rows "For: none" (the
G3 rule, "none: kept", "ours is correct"); an issue per row would be a new
rule for the user (D-008/D-020).

`Point: AmiBlitz3 deviations keyed by their variant.md row — L2 Possible / I2 Medium / E2 Reasoned → Plan`

### D7. Test files: grouping and names

- Directory `tests/ab3/model/`, `tests.toml` with
  `[matrix] compiler = ["ab3", "amiblitz3"]`, `runtime-errors = "on"`
  (the cases without `-r`; blitz2-ab3 then takes the source's header or its
  own default, as `ab3_cli`); prefix `ab3_`; registered by the build step of
  `cmake/basic-test-model.cmake` as `tests/blitz2` is.
- One file per topic and compiler state: `OPTIMIZE`/`SYNTAX` and the AB3
  header hold for the whole driver, so cases are grouped by them
  (`proc-opt4.test.bas`, `proc-nonewsyntax.test.bas`, `esc.test.bas`, ...).
  The case name is the `ab3_cli` case name (`!TestCase{proc_defaults}`), so
  `ab3_proc_opt4__proc_defaults` keeps the old key.
- A case that was not `REF` (expr_depth, the `*_bb2` cases) excludes the
  `amiblitz3` cell (`exclude`), as it was not in `ab3_cli_ref`.
- A file of its own for: a case with a `.ref.expect` "no executable"
  (D4); a case whose subject is a name of the program (label_ns, macro_ns,
  label_proc, underscore, dotlabel, proc_fastname: their names are not
  renamed); a case with an AB3 header (D9); a case that asserts the output
  (D8, which runs on its own anyway).
- In a shared file procedures and macros get a prefix of their case where
  two cases clash (the spike's `fn_`, `fs_`, ...); `SHARED` is written in
  upper case (mixed case is "Garbage at End of Line" in AB3 mode, both
  compilers); a `FAST` Statement passes its value through a Long shared
  variable, never a string (both compilers refuse strings there).

`Point: grouping and renaming rules — L2 Possible / I1 Low / E3 Proven (spike ab3cli20.test.bas, report item 3) → Monitor`

### D8. NPrint lines become checks, unless printing is the subject

The check part is a Plan choice; the form of the output cases is the
user's: Q3 of rounds/tree decisions, "2026-11-27 ab3-test-model Q1-Q5", "Eigener Test, Ausgabe wörtlich
(Recommended)".

A case whose `[run]` lines are values (numbers, strings the program
computes) gets one check per line, its expected value the line as
written: `!CheckEq` for a whole number, `!CheckQEq`/`!CheckFltNear` only
where the line is a Quick/Float the case computes (with the line's
digits), `!CheckStrEq` for text; the check's name is the line's position
(`l1`, `l2`, ...) or the line's own key where it has one. The final
`rc=0` is the default assertion.

A case whose subject is printing itself (the escape sequences of
strings printed, Print's formatting of Quick/Float values, `Print`'s
own behaviour: esc_*, chr_float, pow_negative_base, proc_defaults and
the others D10 marks "output") keeps its `NPrint` lines and asserts
`output == "..."` with the `[run]` lines of its expect file verbatim, the
form the expression matrix uses for its Print cells (spec
`testing/expression-matrix`, "The matrix's tests are tests of the BASIC
test model"). Such a case runs on its own (requirement "Cases that run on
their own"), one session per case and side; its time is measured in
task 8.3 (Q3 of the same entry).

### D9. testpp: `@@` and the AB3 header

- In `source-mode = "ab3"` testpp joins a line ending in `@@` (outside
  strings and comments, AB3's rule as `ext_nextline`/`findjoin` of
  `programs/blitz2/src/ab3syn.c` read it) with the next for its own
  parsing, so a check macro may be continued with `@@`. A check macro so
  continued is written to the driver on one line (number inserted, name
  removed) followed by empty lines for the joined ones, so the driver's
  line numbers stay those of the source. Lines without a check macro are
  written as they are, with their `@@`: the compiler does the joining. In
  BB2 mode `@@` is not special (the driver is unchanged byte for byte).
- A case whose subject is `@@` (join, join_op, join_funcpar, chr_at,
  macthen_join) keeps `@@` in its own statements, outside the check macros.
- The leading header block of an AB3 test source (comment lines `; KEY =
  VALUE` before the first other line) is written as the first lines of the
  AB3 driver, before the macro text, where AmiBlitz3 and blitz2-ab3 read
  it. Without such a block the driver starts as today.

`Point: testpp @@ and header — L2 Possible / I2 Medium / E3 Proven (spike report item 2: "the line ends before the closing }") → Plan`

### D10. Which cases move

Decided by the user: Q1 of rounds/tree decisions, "2026-11-27 ab3-test-model Q1-Q5", "~97 + 4 -r-Fälle
(Recommended)": about 97 run cases and the 4 hand-written `-r` cases move;
the 19 AmiBlitz3-shipped test programs, the 19 input-file cases,
macrobuf_xtra and noend stay `ab3_cli`.

Move (about 97 + 4): the 132 run-output cases without arguments and the
5 hand-written cases with `-r` (`runtime-errors = "off"`; the fifth,
windowtags, is one of AmiBlitz3's programs below), except:

| Stays `ab3_cli`/`ab3_cli_ref` | Cases | Reason |
|---|---|---|
| AmiBlitz3's own test programs, fetched | atoi, compilersignals, data_restore_in_function, fabrizio_speedquestion, function_problem, hexdigits, hex_error, int_problem, int_tests_nostop, jsr, kick13, localization, macro_test, memoryleak_test, optimizer_regression, setint, test_newtypes, trim, windowtags (`-r`) | they come from `archive/amiblitz3/` (`cli.cmake` reads `AB3/Sourcecodes/Amiblitz3/Tests`); rewriting them into `Case` blocks would check a derived copy into the repository (rule 7, "License": AmiBlitz3's licence is unclear) |
| The compiler's input files | inctext, inctext_bbname, inctext_bb2, incfallback, inctok_ab3, incxtwice, incitwice, join_incok, join_incmacro, incguard, esc_codeline, esc_codeline_hdr, esc_codeline_join, esc_include, inc_longlong, macend_inc, inc_error, resident, resident_path | the subject is the lookup of an include or resident by its name next to the main source; in a driver on `CORPUS:` with the files under `CORPUS:ID.test/` the names would change, and with them what the case tests |
| The source form itself | macrobuf_xtra (a tokenised main source with its `.xtra`), noend (a program without `End`: a driver always ends) | a driver cannot have that form |

The `output` cases of D8 are marked in the conversion table of task 5.1.
The expression matrix's 54 cases move too, by its generator (group 7):
Q2 of the same entry, "Ja, jetzt (Recommended)".

### D11. AmiBlitz3 without Defaultdbug

The `amiblitz3` cell has no `Defaultdbug` (not fetched): a test with
`!ExpectError` or a check that needs runtime error checking excludes the
`amiblitz3` cell, and the registration refuses `!ExpectError` in a test
with an `amiblitz3` cell, naming file and line. The command is the one of
`ab3_cli_ref` (no `-r` for `runtime-errors = "on"`), so AmiBlitz3's
results are those `ab3_cli_ref` records today (variant.md row
`debugconst_on`: `#__debug` 0 without the debugger).

`Point: no Defaultdbug on the AmiBlitz3 side — L1 Unlikely / I1 Low / E3 Proven (spike (b), variant.md:840) → Monitor`

### D12. How "only the form changes" is shown (CLAUDE.md rule 3)

1. No product result changes: no source of blitz2, blitz2-ab3, TED or a
   library is touched; every commit of the change is checked with the prev
   build without `PREV_INTENDED` (`prev_results`, the model's prev
   comparisons, the remaining `ab3_cli_prev_N`).
2. The testpp change leaves every existing driver byte-identical: the
   drivers the build writes to `model-drivers/` for `tests/blitz2` and
   `tests/bug-orig` are compared before and after (a host test of testpp
   with a BB2 source holding `@@`, and the prev comparison of the model
   drivers, whose executables must not change).
3. Every moved case checks the values of its expect file: in the commit
   that moves it, (a) the old `ab3_cli` case and the new test pass on the
   same build before the old one is removed; (b) the conversion table
   (task 5.1, kept in the work directory and attached to the report) maps
   each `[run]` line to one check and its expected literal, or to the
   verbatim `output` assertion; (c) a one-off comparison in the scratchpad
   (D-002: one-off analysis, not committed) reads the new result file's
   `check` entries and the old expect file and finds every `[run]` line as
   the expected value of exactly one check, in order; (d) for the
   AmiBlitz3 side the same with `.ref.expect` against the deviations, in
   the preset `oracle`.
4. The compiler sees the moved code in the drivers: the prev comparison
   (D13) compiles them with both commits' blitz2-ab3.

### D13. Prev comparison

The tests of `tests/ab3/model/` join the prev comparison of the model
(`PREFIX_ID_prev`, spec `testing/previous-commit`): the driver of all
cases compiled by the current and the previous blitz2-ab3, once per value
of `runtime-errors` among the `ab3` cells; the `amiblitz3` cells are not
compared (AmiBlitz3 never changes). Entries `ab3_ID_prev:ID` /
`ab3_ID_prev:ID-runerrs`. The AB3 drivers do not join
`BASIC_TEST_DRIVERS` of `ab3_bb2_equivalence` (they are no BB2 sources).
`ab3_cli_prev:CASE` stays for the remaining `ab3_cli` cases.

`Point: prev comparison of the AB3 drivers — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor`

### D14. Bundles of `ab3` cells

The requirement "The cases that may share a session with other sources"
allows, besides the default session key, the key of `compiler = "ab3"`
with otherwise default settings: `ab3` cells of several sources share a
session. Reason: the user's answer of 2026-11-23 ("Ein Test nach der neuen
Methodik sollte im Idealfall nicht langsamer sein als die alten Tests"),
and the spike's ~15 ms per bundled case against ~90 ms in a block.
`amiblitz3` cells are not bundled for now: Q4 of rounds/tree decisions, "2026-11-27 ab3-test-model Q1-Q5",
"Nein, vorerst nicht (Recommended)"; all cases of one test file still
share its run.

`Point: bundles of ab3 cells — L2 Possible / I2 Medium / E3 Proven (spike timing table) → Plan`

### D15. Order with the other open changes; shared files

- `ab3-mode-g5` (recorded: this change first). Shared files:
  `tests/ab3/CMakeLists.txt`, `tests/ab3/cli/`, `tests/ab3/cli.cmake`
  (g5 1.2 reads it), `tests/corpus/runner/` (g5 1.2's pass in
  `cmd_cd.cpp` stages the same tree: it should call D3's function),
  `docs/ab3/variant.md`, `docs/testing.md`, `tests/README.md`. g5's tasks
  are adjusted by task 9.1: only g5's new run-output cases take the new
  form, its error/warning cases (tasks 2.5, 4.3, 5.1) stay `ab3_cli` (Q5 of
  rounds/tree decisions, "2026-11-27 ab3-test-model Q1-Q5", "Nur Ausgabefälle neu (Recommended)", which
  settles the reading of the entry "tests/ab3 route B: now, before
  ab3-mode-g5, as an OpenSpec change").
- `console-break-option` (task 2.2 adds `ab3_cli` cases with
  `-r -ctrlcbreak`, which stay `ab3_cli` by D10): shares
  `tests/ab3/CMakeLists.txt` and `tests/README.md`; no order needed, a
  merge of the case lists.
- `blitz-coverage` (task 3.2: the coverage switch for the `ab3/cli` and
  corpus runners): after this change the moved cases run through
  `corpusrun case`; its task 3.2 must cover the `ab3` cells too (a note for
  the coordinator, task 9.2). Shares `tests/ab3/CMakeLists.txt`
  (`AB3_CLI_BLOCK` with `BLITZ2_COVERAGE`).

### D16. bug-port tests of AB3 mode

A run-output defect of AB3 mode found from now on is a test of
`tests/ab3/model/` with `labels = ["bug-port"]`; a compile-step defect
keeps the one-case `ab3_cli` form. One sentence in `tests/README.md`,
"bug-port tests".

`Point: bug-port form of AB3-mode defects — L2 Possible / I1 Low / E2 Reasoned → Monitor`

## Risks / Trade-offs

- [AmiBlitz3 hangs silently in ~1 of 45 sessions] → the runner's rerun of
  an open case; `amiblitz3` cells keep `cli.cmake`'s time limit scale
  (`timeout` in `tests/ab3/model/tests.toml`), `hang-after` set from a
  measurement (task 3.4). Monitor.
- [Output cases run alone: more sessions than the blocks] → measured in
  task 8.3 (Q3).
- [128-character lines] (follow-up pitfall 4): converted lines are kept
  below the limit of the old sources; testpp's own line limit applies.
  Monitor.
- [A changed testpp breaks an existing driver] → D12 point 2.
- [The `variant` row name drifts] → the build check of D6.
- [Fewer cases in `ab3_cli` blocks change their block cut] → block
  numbers of `ab3_cli_N` move; `PREV_INTENDED` names cases, not blocks, so
  nothing breaks.

## Migration Plan

Infrastructure first (groups 1-4), a pilot of five cases (group 5), the
moves by topic (group 6), the expression matrix (group 7), docs and
measurement (group 8). Each move commit removes the old cases it replaces;
a revert of that commit brings them back. Rollback of the infrastructure is
a revert of its commits; no data format of results changes.

## Recorded answers

Q1 to Q5 of the planning report are answered: rounds/tree decisions, "2026-11-27 ab3-test-model Q1-Q5"
(Q1 D10, Q2 D10 and group 7, Q3 D8, Q4 D14, Q5 D15). The reading of
D9 (the AB3 header in the driver is the compiler's input under test, not
a run setting the runner takes from the source) is kept. The planning
agent's Plan and Monitor choices are recorded in the coordinator-decisions
file of the round.

## Implementation notes

- D9, `@@` (task 1.1): testpp joins at the first `@@` outside strings and
  comments anywhere in the line, the rest of the line ignored, as
  `findjoin`/`ext_nextline` do (not only at the line's end). A joined line
  that holds a check macro is written whole on its first line, so other
  statements of that joined line lose their `@@` too (the compiler sees
  the same joined text). A `@@` in the last line is refused for any line,
  not only for a check (the compiler's "No line to join with @@"; in the
  driver the line would join `.tc__skip`). The form check
  (`tp_check_form`) reads the joined lines as one line as well.
- D9, header (task 1.2): the header block is the run of comment lines
  the source starts with, up to the first other line or up to the line
  that ends AmiBlitz3's embedded header (`/XTRA` by the rule of
  `tokenizer_ab3_header`), and only if one of its lines reads
  `; KEY = VALUE`; so the real headers of the cases (`; XTRA`, `; Version
  = 1.2.3`, `; /XTRA`) are written whole, and a source that starts with a
  plain description comment gets no header. The lines stay in the source
  part of the driver as comments, so no line number moves.
- D2/D6/D11 (task 2.1): `source-mode` has no written default (`ab3` is
  what a cell of `ab3`/`amiblitz3` without it reads, `tc_cell_ab3`), so
  `--print-config` of the existing tests is unchanged. `variant` is kept
  apart from the queries (`tc_config.variants`, one per selector); a
  table whose selector matches cells of `amiblitz3` and others is refused
  (its other cells would carry a variant). A deviation with a variant is
  no deviation of the original: `fixed-orig`/`extension` still need one of
  those, `fixed` may stand beside it. The `!ExpectError` refusal is
  `tc_expect_error_ok` in testconf (the matrix), called where the source
  is read (testreg, task 2.2): testconf does not read the source. The
  side of a cell: `amiblitz3` is `oracle` (not `originals`); `ab3` is
  `port` unless `libs = "cd"`.
- D5 (task 2.2): testreg reads a source in the source mode of its cells,
  so it builds the matrix before the source; a test whose matrix fails
  now reports that error alone (before, its source errors came first).
  A test with cells of both modes (a `port` cell and an `amiblitz3` one)
  is read in both, each message once. A test whose deviations are all
  variants is judged like a fixed one (`tj_verdict`: the `amiblitz3`
  cells with their deviations, the others plain; never REPRODUCED). The
  registration of `tests/ab3/model/` is in place; it registers nothing
  until the pilot adds sources.
