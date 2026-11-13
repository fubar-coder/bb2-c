# Design

## Context

Observed state on main:

- 213 expect files in `tests/corpus/expect/{corpus,corpus_runerr,corpus_req,
  corpus_serial,corpus_serial_runerr}/` (`NAME.out`, serial passes also
  `NAME.serial`); the program's output with an `rc=N` line, or the bytes the
  program sent over the serial port.
- A pass is two ctest tests (`tests/corpus/CMakeLists.txt:40-62`). `NAME`
  (`all`): `clib` (blitz2-c with the C libraries) and, with a prev build,
  `prev` (its executable must equal clib's, result changes via
  `PREV_INTENDED`). `NAME_originals` (`originals`, CD): `ref` (original 2.10
  with the CD's libraries), `port` (blitz2-c with the CD's libraries; its
  executable must equal ref's), `orig-clib` (original compiler with the C
  libraries). Every configuration's output must be the expect file, except
  where `notes.tsv` lists the program.
- The runner (`tests/corpus/runner/corpus.c`) reads `expect_dir=`, `notes=`,
  `notes_pass=` pass keys; `corpus_update` (`:1371`) writes `expect_dir` from
  one configuration (`corpusrun pass --update-expect clib`), target
  `corpus-expect-update` (`tests/corpus/CMakeLists.txt:213-222`).
  The pass key `layout=stack` (`corpus_layout`, from the change
  bug-orig-test-model) gives the compiler and the programs a Stack 16384
  bytes larger, the other memory layout of the bug-orig recording; it is
  available here too.
- The notes judge is `tools/lib/corpusnotes` (`cnotes_parse`, `cnotes_judge`,
  `cnotes_stale`), with a host test and the ctests `corpus_notes_*`,
  `corpus_originals_notes_*` (`tests/corpus/CMakeLists.txt:313-365`).
- Today's rows: `notes.tsv:20-33` (14 rows: issues #46 and #6, and the
  library fixes #1000 to #1004 in `corpus` and `corpus_runerr`).
- The libs judge already implements case selection by qualifier, accept of
  two runs and fix-scope (`tools/lib/libscmp`).
- The review of 2026-11-12 (section 2D) noted that a two-run accept adds
  little for the corpus because every pass already runs the same source in 2
  or 3 configurations against one file; it stays valid for the writing
  from the original (`ref` is one configuration only).

## Goals / Non-Goals

**Goals:** corpus expected data in the shared model (a fixed bug = two
cases, no side table); the judge reads them; record from the original with
agreement of two runs; fix-scope covers the corpus; all five configurations
keep their meaning.

**Non-Goals:** `corpus_cd` and its table, the TED oracle, `ted_ab`,
`PREV_INTENDED`; changing what a configuration compiles or runs; changing
programs or their sources.

## Decisions

### D1. Cases, keys, files

A case is one observation of one program in one pass: key =
`PASS|NAME|OBS|1` (routine = the pass, check = the program, vector = the
observation `out`, `serial` or `exe`; the occurrence number is always 1; the
form of `libscmp.h`), value = the file's content. Files in
`tests/corpus/expect/PASS/`:

| File | Case |
|---|---|
| `NAME.out`, `NAME.serial` | `PASS|NAME|out|1`, `...|serial|1` unqualified: original and port agree |
| `NAME.orig.out`, `NAME.orig.serial` | the same key `[orig]` |
| `NAME.fixed-N.out`, `NAME.fixed-N.serial` | the same key `[fixed #N]` |
| `NAME.orig.exe`, `NAME.fixed-N.exe` | key `...|exe|1` `[orig]` / `[fixed #N]`: SHA-256 of the executable of `ref` / `port` (text, one line) |

A key has either the unqualified file or the qualified ones, never both,
and at most one `fixed-N` file (the malformed rules of `libscmp.h`). A
program whose `ref` and `port` executables agree has no `.exe` file (the
agreement stays a live comparison in `originals`, as today). An executable
case exists only for a fix that changes the code. The notes row of #46 on
`corpus:t130_blit` ("other code", same output) becomes exactly such a pair
of `.exe` files without `.out` pair.
The `.exe` cases are the one exception to "`[fixed #N]` runs in every
`all`": the port's executable is made with the CD's libraries, so they
are judged in `NAME_originals` only (state in the docs). The directory is
the unit the judge reads; there is no new data file.

### D2. Which case applies to which configuration

| Configuration | Run in | Compiler / libraries | Applies |
|---|---|---|---|
| `clib` | `NAME` (`all`) | port / C libraries | unqualified, `[fixed #N]` |
| `prev` | `NAME` (`all`, prev build) | previous commit / its libraries | as `clib` (its output must be the same as clib's: a result the commit changed shows via `PREV_INTENDED`, unchanged) |
| `ref` | `NAME_originals` | original / CD's libraries | unqualified, `[orig]` |
| `port` | `NAME_originals` | port / CD's libraries | unqualified, `[orig]` or `[fixed #N]` (either) |
| `orig-clib` | `NAME_originals` | original / C libraries | unqualified, `[orig]` or `[fixed #N]` (either) |

`port` and `orig-clib` mix one original and one ported part; whether a fix in
the compiler or in a library shows depends on the bug, so the judge accepts
either value there and reports which one matched (`matched [fixed #6]`).
`ref` fully original must give `[orig]`, `clib` fully ported `[fixed #N]`; so
a changed original fails `[orig]` and a regressed port fails `[fixed #N]`.
A case that neither value meets fails naming the key and the first differing
line, as today. `exe`: `port` executable against `[fixed #N]` hash, `ref`
against `[orig]` hash; without a pair, port equals ref as today.

### D3. The judge

`tools/lib/corpusnotes` becomes `tools/lib/corpusexpect`: it lists the files
of a program in `expect_dir=`, builds the cases (D1), selects per
configuration (D2) and reports by key. It reuses `libscmp` for the
qualifier rules (malformed, ambiguous) by projecting the directory onto a
`libscmp_file` in memory (the same projection serves fix-scope, D5): no
second parser of qualifiers. Reports keep the pass-file output of the runner
(`OUTPUT DIFFERS`, first differing line); "noted" and "STALE note" are
removed. A stale pair is caught by its own configuration (a changed original
fails `[orig]`, a regressed port `[fixed #N]`); only a pair of equal values
is refused as malformed (it would mean no difference). The pass keys `notes=` and
`notes_pass=` are removed.

### D4. Record and update targets

- `corpus-expect-update` (unchanged: from the port, `clib`, no CD for the
  output where the programs allow it): rewrites the unqualified and
  `[fixed #N]` files from `clib`. If a program's unqualified file would
  change, the update refuses and names the key unless
  `CORPUS_UPDATE_FIX=PASS:NAME#N` is given, which turns that unqualified file into `NAME.orig.EXT` (old
  content) and `NAME.fixed-N.EXT` (the port's), as `libs-update`. A new
  program has no files: the update writes them unqualified. A change of
  an `[orig]` file is never made by the update.
- `corpus-expect-record` (new; needs Amiberry and the CD;
  `CORPUS_RECORD_FILTER=PASS:NAME`):
  runs `ref` twice, the second time with a Stack 16384 bytes larger
  (the layout of the bug-orig recording, `corpus_layout`), accepts only if
  both runs agree (`results corpus-accept RUN1 RUN2 DIR`), else "test not deterministic: PASS:NAME"
  naming the differing lines and writing nothing; it writes the unqualified
  and `[orig]` files and keeps the `[fixed #N]` ones. The proof is limited
  to the memory layout.
- The `.exe` files are written by `corpus-expect-record` (`ref`) and
  `corpus-expect-update` with `FIX` (`port`, which needs the CD's
  libraries, so only with the CD).

### D5. fix-scope

`cmake/fix-scope.cmake` adds `tests/corpus/expect`; `results fix-scope`
projects the directory (base from `git archive`, branch from the checkout)
onto cases (D1), a value being `bytes N fnv H` for the files (a file's
text is the diff's business: the message names the key and the hashes and
the commit's `git diff` shows the lines), and calls `libscmp_fix_scope`
unchanged. A fix with `Fixes #6` that turns `corpus_runerr:t124_bitmaperr`
into its pair passes; any other changed file is reported. A branch that only
adds new programs reports them as added (review), as new libs cases do.
Because `libscmp` is being changed by `libs-capture-host-blocks`, task 1.1
re-reads the interface.

### D6. Migration

The 14 rows become files in one commit (task 5): for each, `corpus-expect-record`
produces the `.orig` file from `ref`, and the current expect file becomes the
`[fixed #N]` file (`git mv` is avoided by writing both and removing the old
unqualified one, so the diff shows the pair). `t130_blit` in `corpus`
gets the `.exe` pair, and the `corpus_runerr` one the `.out` pair (+ `.exe` if
the executables differ). Acceptance: the `prev` comparison shows no
executable changed; `ctest --preset originals` passes without `notes.tsv`.

### D7. Selection

`tools/testinfo/rules.txt` already selects the corpus tests for
`tests/corpus/expect/**` (`:72`) and `tests/corpus/**` (`:77`); no change.

## Risks / Trade-offs

- A file per observation and qualifier makes more files in the directory
  (only for fixed programs: 14 rows today, so about 30 files).
- `port` and `orig-clib` accept either value: a regression in them that
  restores the original's value is not caught by those two; `clib` catches
  it (the same source, same fixed code in the port's side).
- The `.exe` case is judged only with the CD (exception in D1).
- Hashing outputs in fix-scope hides the lines; the report points to
  `git diff`.

## Findings (implementation)

- **1.1, the interfaces re-read (2026-11-13).** `libscmp.h` on main has the
  fix-scope interface D5 names unchanged (`libscmp_issues_*`,
  `libscmp_fix_scope`, `libscmp_parse` with `LIBSCMP_EXPECTED`); the
  change bug-orig-test-model added `tools/lib/bugorigcmp` with the same
  pattern for a tree (`bugorigcmp_project_tree`,
  `bugorigcmp_fix_scope_trees`), which `cexp_project_tree` and
  `cexp_fix_scope_trees` follow. `bugorigcmp_accept` and
  `libscmp_accept` work on section files and case files; the corpus
  record compares whole files (an output, serial data, a hash), so its
  two-run check is a byte comparison with the first differing line
  (`cexp_accept`), with no projection: a projection would only hide the
  line. The qualifier rules are those of `libscmp.h` (one unqualified or
  the pair, at most one `fixed`), checked on the file names by
  `corpusexpect` so that a message names the files; the projection onto a
  `libscmp_file` serves fix-scope only.
- **The second layout is a larger Stack, not inject's `pad`** (from the
  change bug-orig-test-model, question 10 of the questions file): `pad`
  changed what the tests observe. D4 and task 3.2 say so.
- **14 rows instead of 4.** Between the planning and the implementation,
  main got the library fixes #1000 to #1004 with ten more notes rows
  (`corpus` and `corpus_runerr` each); they become pairs as the four
  older ones (task 5.1).
- **A run's set.** The record reads two directories of a run's
  observations (`corpusrun pass --observe`): plain files, `NAME.exe` the
  hash of the executable; `CEXP_RUN` allows exactly that, while the
  expected data allow an `.exe` file only as part of a pair.
- **A lone file of a pair** is malformed for the judge (strict) but
  allowed while writing: the migration writes `NAME.fixed-N.out` from the
  committed file first and lets the record complete the pair with
  `NAME.orig.out`.

- **The variables of the targets** are cache variables, as those of the
  other targets (`LIBS_UPDATE_FIX`, `BUG_ORIG_RECORD_FILTER`):
  `CORPUS_UPDATE_FIX=PASS:NAME#N,...` and `CORPUS_RECORD_FILTER=PASS` or
  `PASS:NAME,...`; the spec and D4 name them so.
- **The executables of the update.** `corpusrun pass --update-expect port`
  on a pass of `NAME_originals` runs `ref` and `port` for the programs with
  an exe pair or a FIX only (none: no session, exit status 0); without the
  CD it is skipped, which fails the target only if a FIX names that pass.
- **An observation works in a directory of its own** (`DIR.work`, report
  `DIR.txt`): the two runs of the record use one pass file at the same
  time, and with the pass's `work=` they wrote into one directory (found
  in the first migration run: every program of `corpus_runerr` "did not
  compile"; `corpus` happened to pass, so it was recorded again).
  `serial_same_as=` is dropped in an observation: it judges, the record
  only observes.
- **`results fix-scope --fixes` once only** (a bug found here): the option
  had no multi-option policy, so a second `--fixes` was refused; task 5.1's
  check needs several. Fixed in its own commit, with a test.
- **The migration (5.1)**: of the 14 rows, `corpus:t130_blit` (#46) became
  the executable pair only (its output is the same); the other 13 became
  output pairs, and `corpus_runerr:t130_blit` also got the executable
  pair. All other fixed programs' executables of `ref` and `port` agree
  (the update said so for each FIX). `orig-clib` matches `[fixed #N]` for
  the library fixes and #6, `[orig]` for the compiler fix #46, as expected.
- **5.2**: three full records in a row (all 215 programs of the five
  passes, two runs each) accepted every program and changed no file.

## Open Questions

In `~/.local/share/bb2-c-work/questions-for-user.md`: the executable pair as
hash files (planned: yes), and whether `port` / `orig-clib` accept either
value (planned: yes).
