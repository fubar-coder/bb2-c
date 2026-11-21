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
OK"). Three tasks and no more: it numbers the checks per namespace (case,
procedure, `init`, `end`) and writes the number into the call; it keeps the
table of namespace, number, name, line and expression text for the report
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
  (`[asserts."libs=cd"]` is the working form).
