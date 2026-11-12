# Proposal

## Why

The shared test model (`openspec/specs/testing/test-model/spec.md`) is
implemented for the library tests; `docs/testing.md` lists the corpus as a
planned follow-up ("the notes rows become `[orig]` / `[fixed #N]` cases").
Today the corpus keeps the port's result in `tests/corpus/expect/PASS/NAME.out`
and the intended difference from the original in a separate table,
`tests/corpus/notes.tsv`, read only by the runs of `originals`
(`tests/corpus/CMakeLists.txt:80-82`, `:149-150`; judge
`tools/lib/corpusnotes`, words "noted #N" and "STALE note"). The table
has 4 rows (`notes.tsv:20-23`: `corpus:t130_blit` #46,
`corpus_runerr:t130_blit` #46, `corpus_runerr:t124_bitmaperr` and
`t126_rgberr` #6). It does not say what the original gives, so the original's
value of a fixed case is not data anywhere; the model puts it in the test
definition (`KEY [orig]` next to `KEY [fixed #N]`), where a changed original
and a regressed port each fail on their own case. The corpus is also not
covered by `fix-scope`, so a fix that edits an expect file is not checked
against its issue. Two recorded properties are missing as well: a recording
(`corpus-expect-update` writes from the port; nothing writes from the
original) and the two-layout agreement the model asks of a record.

## What Changes

- The corpus expected data becomes the pair for a fixed program: the
  unqualified files `NAME.out` / `NAME.serial` stay where original and port
  agree; a fixed program has `NAME.orig.out` (original, from `ref`) and
  `NAME.fixed-N.out` (port, issue N) instead, and likewise for the `.serial`
  files and for the executable (a hash file `NAME.orig.exe` /
  `NAME.fixed-N.exe`, design D3). `tests/corpus/notes.tsv` and the notes
  judge are removed.
- The judge (`tools/lib/corpusnotes`, renamed `corpusexpect`) selects, per
  configuration, the file that applies (table in design D2) and reports
  differences by key as the libs judge does; no second notion of "stale":
  a regressed port fails `[fixed #N]`, a changed original fails `[orig]`.
- Targets: `corpus-expect-update` (from the port, as today; changed
  unqualified cases are only taken with `FIX=PASS:NAME#N`, which turns the
  case into the pair, as `libs-update`), and a new `corpus-expect-record`
  (from the original, `ref`, two runs in a different memory layout that must
  agree, else "test not deterministic"; filterable) that writes unqualified
  and `.orig` files and keeps the `[fixed #N]` ones.
- `fix-scope` covers `tests/corpus/expect`: the files are projected onto
  cases and the existing libscmp fix-scope judges them.
- The configurations stay (`clib`, `prev`, `ref`, `port`, `orig-clib`);
  design D2 says what each judges.
- Documents: `docs/testing.md`, `tests/README.md` ("Corpus", the notes
  paragraph is replaced), `tests/corpus/runner/README.md`, the site pages
  `site/content/docs/project-structure/tests.md` and
  `site/content/docs/compared-with-original/comparing.md`, `TODO.md`, the
  header comments of `tests/corpus/CMakeLists.txt`.

## Capabilities

### Modified Capabilities
- `testing/expected-results`: corpus expected data as cases (pair for a
  fixed program), targets of record and update.
- `testing/original-comparisons`: the `originals` runs of a corpus pass judge
  against the expected data cases instead of notes rows.
- `testing/bug-fix-workflow`: fix-scope covers the corpus expected data.

## Impact

- `tests/corpus/CMakeLists.txt`, `tests/corpus/runner` (`corpus.c`,
  `corpus.h`, `cmd_pass.cpp`), `tools/lib/corpusnotes` (becomes
  `corpusexpect`), `tools/results` (`fix-scope` extension, an accept
  subcommand for the record), `cmake/fix-scope.cmake`, the 4 notes rows are
  migrated to files (task 5.1), `tests/corpus/selftest/`.
- Dependency: another agent is changing `tools/lib/libscmp` right now
  (`libs-capture-host-blocks`); this change plans against the fix-scope
  interface as on main (`tools/lib/libscmp/libscmp.h:214-258`); task 1.1 reads
  it again. It also shares the C code of the change `bug-orig-test-model`
  (the two-run accept, the projection helper); apply that change first
  (its `testing/original-comparisons` text is the base of this change's
  delta).
- Results (executables, TED files, libraries): unchanged; acceptance stays
  byte-identical to the `prev` build. `corpus_cd` (`tests/data/blitz2-cd.tsv`),
  the TED oracle and `prev` (`PREV_INTENDED`) are not changed.
- No program behaviour changes; only tests and test tools.
