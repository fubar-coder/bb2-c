# Design

## Context

See proposal.md, "Why". What exists (details and measurements:
`~/.local/share/bb2-c-work/tr-open-points-review.md`):

- **`corpusrun`** (`tests/corpus/runner`): `corpusrun pass PASSFILE`
  compiles and runs the sources of a pass file in Amiberry with several
  configurations in one session and judges them; in the bug-orig mode its
  exit status is the worst verdict (0 FIXED, 1 REPRODUCED, 3 BROKEN,
  4 DIFFERS). The pass file (`key=value`, keys in `corpus.h`) is written by
  CMake from the header directives (`tests/bug-orig/CMakeLists.txt:189`,
  `file(READ)` and regular expressions; configurations `ref`, `port`,
  `clib` at `:378-388`; `TIMEOUT`, `COST`, `FIXTURES_REQUIRED` at
  `:451-456`, `:471`, `:487-503`; placeholders `@TESTDATA@`, `@USERLIB@`,
  `@CD@` at `:215`).
- **The directives today** (`tests/**/*.bas`): `@fixed` 955, `@pass` 836
  (`timeout` 235, `file` 198, `runtime` 135, `runwrap` 93, `jobline` 56,
  `wrap` 32, `uae` 26, `serial` 23, `dir`, `floppy`), `@rwe` 519,
  `@configs` 316 (always `clib`), `@machine` 56, `@runerrs` 17,
  `@hang-after`, `@file`, `@want`.
- **Sessions:** a session starts from a savestate keyed by a hash of the
  configuration (about 1.9 s per restored session, 716 sessions in
  `junit-all.xml`); a hard limit of simultaneous sessions (slots) is shared
  by all runners. Batched compile and run costs 0.25-0.33 s per program.
- **Hand-made matrices:** the seven `tests/blitz2/*-runerrs.bas` have the
  same code and `.expect` as their twins; the corpus runs as `corpus` and
  `corpus_runerr`.
- **Names:** the test selection maps a test to its tracefile by the name
  with every character outside `[A-Za-z0-9_]` replaced by `_`
  (`tools/lib/covmap/covmap.h:23-25`); rules select by prefix
  (`tools/testinfo/rules.txt`); coverage is per session
  (`cmake/emulation-queue.cmake:41-59`).

## Goals / Non-Goals

**Goals:**
- One place for how a test runs and what it must show besides its checks.
- CMake knows file names only; logic is C (CLAUDE.md rule 6).
- The old form and the new one side by side during the move.

**Non-Goals:**
- Moving the corpus, TED A/B, `tests/ab3` or the library capture tests
  (user: "wir sollten nicht **alles** in diese Form pressen").
- Pure compiler tests: a later form through TED's proxy or the compiler's
  command line (user, 2026-11-21).
- Batching several tests in one session, `isolate`, a `CONTAMINATED`
  verdict, restoring `RAM:` between programs: a later change with its own
  spike (user: "Erst pro Case, Bündeln später").
- The move of the bug-orig tests: change `bug-orig-basic-tests`.

## Decisions

**D1 The runner reads the configuration, CMake globs file names.** The
generator of the pass file moves from CMake into the runner. CMake hands
the runner one environment file per build with the paths only it knows
(`blitz2`, the library sets per variant, `inject`, the CD, the
placeholders); build products that today are per test (RWE user library
objects, token lists) are built per variant.

**D2 File names** (user: "ID.test.bas"): `ID.test.bas`, `ID.test.toml`,
`tests.toml` per directory; ids stay lower-case kebab-case.

**D3 Hierarchy like `.editorconfig`**: nearest file wins per key, tables
merge per key, lists replace; search stops at `root = true`. Relative
paths are relative to the file that sets them.

**D4 Assertions replace, keyed by query** (user: "Ergänzen ... mMn
problematischer"). The query is the key (`rc = "== 0"`,
`requester = "count == 0"`, `"check.CASE.NAME" = "== fail"`), so replacing
works per query and a test that changes one query keeps the defaults of
the others, with the same merge rule as every other table. The grammar of
queries, predicates and selectors is written down exactly in the
documentation and tested.

**D5 Matrix with three restrictions** (user: "Ja, einverstanden mit den
Einschränkungen"): axes only in the matrix table, only run settings, the
runner groups cells into sessions. The side "needs original material" (a
cell with the original compiler or the CD's libraries) follows from the
values of a cell; `pad` is a run setting, not a default axis.

**D6 The generated driver** (user idea, 2026-11-21): per run the runner
writes a driver that defines the library's macros, holds the preprocessed
text of the test source (copied, not included, so compile errors keep
their line; user: "Kopieren"), jumps over it, calls the library's start
with the result file's name (user: "In den Treiber"), the batch init if a
case is selected, the selected cases with `Gosub` (cases are subroutines,
`basic-testlib` D3), the batch end. No discovery run (user: "OK, können
wir so machen ... für den ersten Wurf"); a renamed case or one behind
`CNIF` fails the compile by its call. The count check with `CNIF`/`CERR`
of the first version is not needed any more. `corpusrun` prints the
driver on request, as it prints the effective configuration.

**D6a The preprocessor** (user: "dann können wir den Runner auch als
Präprozessor verwenden"; numbering: "Wenn das stabil funktioniert, dann
OK"). Three tasks and no more: it numbers the checks over the whole source
and writes the number into the call; it keeps the table of namespace (case,
procedure, `init`, `end`), number in the namespace, name, line and
expression text for the report
(the text never enters the program, so quotes, length and the memory
layout are no concern); it replaces a fixed list of placeholders and
refuses unknown ones. It splits macro arguments by the compiler's rules
(`findit`, `programs/blitz2/src/compile.c:624`); its logic is in
`tools/lib` with host tests on the cases of spike 1. If the numbering is
not stable, the fallback is the constant counter in the program
(`basic-testlib` D4).

**D7 Registration at build time** (user: "Statisch registrieren"): a host
tool reads the sources and resolves their configurations in the build
step, checks the form of the sources (blocks, their pairing and order, no
executable code outside them, case names `[a-z][a-z0-9_]*` that are no
token, no check in a macro definition of the test; user: "Beide Regeln
prüfen"; after spike 4 also the pairing in source order with the case at
fault, no `CNIF`/`CEND` cutting a case, only a comment after the case
macros on their line; user: "Ja, Build meldet", "Regel in der
Build-Prüfung"), and
writes the ctest include file (`TEST_INCLUDE_FILES`, as
`gtest_discover_tests`). One test per case and side (user: "Erst pro
Case"); names `PREFIX_ID__CASE` / `PREFIX_ID__CASE_originals` (prefix as
today: `bug_orig_`, `regression_`, `blitz2_`); empty sides are not
registered; labels per case (a case with a declared deviation of an open
bug carries `bug-orig`, `testing/bug-orig`).

**D8 Format: a strict subset of TOML** (user: "Ja, ich glaube, dass eine
strikte Teilmenge zu Beginn Sinn ergibt"): one key per line; values
string, integer, boolean or a one-line list of strings; table headers;
comments. It is valid TOML, line-oriented (a tool that sets
`fixed-orig = N` changes one line and keeps the comments), and its parser
in `tools/lib` stays small. "Zu Beginn": the subset may be widened later.

**D9 Validators of the original's deviations are assertions** of the
cells with the original's faulty part (user: "**optionaler** nachfolgender
Validator ... Test-ID und Check-ID"); the declaration itself is the basic
validation.

## Interface

The contract with the test library is the spec
`testing/basic-test-interface` (change `basic-test-interface`, archived
before this change starts). The preprocessor, the driver generator and the
judge are written against it; the judge reads result files with the codec
of `shared/basictest/`, the driver copies its macro text. The staging of
the library (`basic-testlib` task 4.1) belongs to this change's
implementation, after task 3.1, because both change `tests/corpus/runner`
and `tests/bug-orig/CMakeLists.txt`.

## Risks / Trade-offs

- [One session per case costs more than today: about 260 s instead of
  175 s wall time for 1000 files at 12 slots (estimate with 1.5 cases per
  file); batched it would be about 65 s] → accepted for now; batching is
  the next change. A filtered run (one case) has another memory state than
  the full program; tests must not depend on it.
- [The test list is complete only after the build; `ctest -N` before it
  shows no tests of the model] → as with `gtest_discover_tests`;
  documented.
- [Without recordings nothing detects a test that depends on the memory
  layout] → stability by form (guards set explicitly, verdicts instead of
  addresses).
- [Cells multiply runs] → the runner shares a session between the cells
  of one case; a warning above a number of cells per test.
- [The preprocessor misreads a check call, so numbers or texts are
  wrong] → its argument rules are tested against the compiler's; the
  fallback of `basic-testlib` D4.

## Open Questions

- The exact selector syntax for cells in table names
  (`[asserts."libs=cd"]` is the working form; implemented as
  `KEY=VALUE[,KEY=VALUE...]` with `case=NAME`, see "Findings").

## Findings of the implementation

Phase 1 (tasks 1.1-1.4, 2.1, 3.2, 4.1), `tools/lib/testtoml`,
`tools/lib/testconf`; the grammar in `tests/README.md`, "The run
configuration".

Decided by the user (`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, 2026-11-21, "basic-test-runner:
findings of implementer B"), and written into the spec delta: only run
settings with a single value are axes; the nearest file first, then the
most specific selector in it; deviations as assertions in
`[deviations."SELECTOR"]` of the test's own file, its further keys the
validators; the extra refusals of the form check and the preprocessor
(below); `TIMEOUT` the sum of a side's cell time limits and absolute
source paths in the include file; the compiler values `port`, `orig195`,
`orig210`. The `findit` question (a byte with bit 7 hiding the next one)
was answered by a spike (`tr-findit-report.md`): TED stores a Latin-1
character of a string as one byte below $20 or as the pair $90 cc, so the
rule holds for the tokenised source only; the preprocessor reads the text
source, where a byte >= $80 is one character (user decision 2026-11-21,
`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`).

- **Keys.** The run settings mirror the header directives and pass keys
  of today (`compiler`, `libs`, `machine`, `runtime-errors`, `rwe`,
  `timeout`, `hang-after`, `stack`, `serial`, `rtg`, `pad`, `wrap`,
  `runwrap`, `runargs`, `launcherargs`, `files`, `dirs`, `remove`,
  `mount`, `floppy`, `uae`, `jobline`); `labels`, `cost` and `fixtures`
  describe the registered ctest test. `compiler` has the values `port`,
  `orig195` (the build of `blitz2.s` 1.95) and `orig210` (the CD's 2.10),
  `libs` `cd` and `clib`; the defaults are `port`, `clib`, `a1200`
  and `off` (decided: `~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, 2026-11-21, "basic-test-runner:
  findings of implementer B"). Task 3.1 maps the settings to the pass file
  and may add keys.
- **Axes.** A list setting cannot be an axis (the subset has no lists of
  lists), nor can `labels`, `cost`, `fixtures` (one ctest test per case
  and side, not per cell). A key that a nearer file sets and a farther one
  makes an axis is no axis, and the other way round; both in one file is
  an error. Cells are named by their values in the fixed order of the
  keys, `,` between them; a test without axes has the cell `default`.
- **Replacement.** For a query the nearest file wins first, then the most
  specific selector of that file (the number of its terms); two different
  assertions of the same rank are an error. A deviation of the cell beats
  every assertion. The spec says "the nearest file and the most specific
  cell selector"; the order between the two, proposed by the implementation,
  is decided (a test's own statement beats a directory's cell-specific one).
- **Deviations** are assertions in `[deviations."SELECTOR"]` of the test's
  own file; the selector names the cells with the faulty part, the further
  entries of the table (an actual value) are the validators. `fixed-orig`
  without a deviation and `fixed` with one are refused (from
  `bug-orig-basic-tests`, implemented here because the configuration
  holds both).
- **Check queries** are `check.NAMESPACE.NAME[.expected|.actual]`; the
  judge splits `NAME` from the suffix against the names of the table, so a
  name may contain a `.`. A case selector term `case=NAME` restricts an
  assertion to one case.
- **Preprocessor** (`tools/lib/testpp`). It finds a call as `expandmax`
  does (a `!` outside strings and comments, the name, blanks, `{`) and
  splits the arguments as `findit` does; `findit`'s rule that a byte with
  bit 7 set takes the next byte with it belongs to the tokenised source
  and does not apply to the text source (a byte >= $80 is one Latin-1
  character there, see above). It also refuses the same check name twice in a namespace (a
  query could not tell them apart) and a procedure with checks named
  like a case or `init`/`end` (its namespace would be ambiguous). The
  placeholders are `@LINE@`, `@CASE@`, `@TEST@` and `@TESTDATA@` (its
  value a parameter of the run), replaced in code and strings, not in
  comments; the table keeps the text as written.
- **Judge** (`tools/lib/testjudge`). The observations other than the
  result file are data (`struct tj_obs`) that task 3.1 fills from the
  session: session end, compile result and log, exit code, output, serial
  data, inject's report, and the requesters, each with the number of
  result file lines written before it was shown (so a requester can be
  matched with an announcement before it). A requester matches an
  announcement if its text holds the message. A case whose last entry is
  a matched announcement has ended as expected: `end.case` holds and the
  default `rc == 0` does not apply (an assertion of `rc` still does).
  A check without entries is `not-run` and fails no default, which gives
  "not run behind a declared crash". An assertion that names a check
  (by verdict or value) takes it out of the default "every check passed".
  The verdict of a case over a side's cells follows `bug-orig-basic-tests`:
  without deviations every cell passes; with `fixed-orig` the faulty cells
  pass with their deviations and the others without; an open bug is
  REPRODUCED when every cell passes with the deviations (the cells without
  the faulty part as if the selector matched), and fails when such a cell
  passes without them. Exit values as `corpusrun`'s bug-orig mode: 0
  passed, 1 reproduced, 2 error, 3 broken, 4 failed.
- **Form check** (`tools/lib/testpp`, `tp_check_form`). Besides the rules
  of the spec it refuses, as code outside the blocks, a call of one of
  the test's own macros and `INCLUDE`/`XINCLUDE` there (their text is not
  known to the check, and an included file would hide checks from the
  numbering); the declarations allowed outside are listed in `testpp.h`.
  A loop, `Select`, block `If` or `SetErr` must not cross a block's start
  or end. A case named `init` or `end` beside a batch init or batch end is
  refused (its namespace would be that of the batch block).
- **Registration** (`tools/lib/testreg`, `tools/testreg`). A test runs
  `COMMAND... --source ABSOLUTE-SOURCE --case CASE --side port|originals`
  (the subcommand is task 3.1's); its `TIMEOUT` is the sum of the cells'
  time limits of its side plus an extra (the runner groups cells into
  sessions, the bound holds for any grouping), `SKIP_RETURN_CODE 77` as
  the other emulator tests. The depfile names every file read; a file
  that does not exist yet cannot be in it, so task 4.2 must glob the
  configuration files (`tests.toml`, `*.test.toml`) with
  `CONFIGURE_DEPENDS` too, or a new `ID.test.toml` next to an existing
  source is not seen until the next configure.
- **String values by length and CRC32** (decided 2026-11-21,
  `~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, entry "String values: length and CRC32": "ok zu
  hash-variante für string-werte"; scope basic-testlib and
  basic-test-runner). Each string value of a `checkstr` entry carries the
  length and the CRC32 of the whole string (the delta of
  `testing/basic-test-interface` in `basic-testlib`). The judge decides
  `==` and `!=` on a string value by that length and CRC32 against those
  of the assertion's string (`basictest_crc32` of `shared/basictest`),
  also for a cut value; this replaces the earlier rule that `==`/`!=` on
  a cut value cannot be judged. `contains` and `matches` take only a
  value that is not cut, and on a cut one the cell cannot be judged
  (TJ_ERROR naming the assertion). Whether a value is cut is decided per
  value (shorter than its length), not by the entry's last field: in an
  entry with one long and one short value only the long one is cut, and
  `contains` still works on the other. The report shows a cut value as
  `"START" (cut, length N)`. A complete value whose CRC32 does not match
  its bytes is refused by the reader, so `==` cannot pass on a forged or
  damaged line; for a cut value the reader cannot check the CRC32, and a
  wrong one makes `==` fail.
