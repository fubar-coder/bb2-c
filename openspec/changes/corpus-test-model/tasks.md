# Tasks

Each task leaves `ctest --preset host` green (and `ctest --preset all`
where it touches the emulator tests); every check has a test with
deliberately broken input. Apply `bug-orig-test-model` first (it adds the
two-run accept and the projection helper this change reuses).

## 1. The judge (host side)

- [x] 1.1 Re-read the fix-scope and parse interface of `tools/lib/libscmp` on main (`libscmp.h`; the change `libs-capture-host-blocks` may have changed it) and the helper of `bug-orig-test-model` (`tools/lib/bugorigcmp`, `results bug-orig-accept`); note differences in the design. Verify: D3 to D5 name only functions that exist.
- [x] 1.2 Replace `tools/lib/corpusnotes` by `tools/lib/corpusexpect`: list the files of a program in an expect directory, project them onto a `libscmp_file` (keys `PASS|NAME|OBS|1`, qualifiers from the names), select by configuration (D2), refuse malformed directories (unqualified plus qualified, two `fixed`, unknown qualifier, equal pair). Verify: host test from memory (every row of the D2 table, each malformed form with the file name, an `exe` pair).
- [x] 1.3 The two-run accept for the corpus (`results corpus-accept RUN1 RUN2 DIR`) on top of the helper of `bug-orig-test-model`. Verify: host tests for equal runs, one differing line (nothing written, "test not deterministic: PASS:NAME"), a kept `[fixed #N]` file.

## 2. The runner reads cases

- [x] 2.1 `tests/corpus/runner` (`corpus.c`, `corpus.h`, `cmd_pass.cpp`): judge each configuration against the applying file(s) through `corpusexpect`; remove the pass keys `notes=` and `notes_pass=` and the words "noted" / "STALE note"; `exe` cases for `port` and `ref` (SHA-256 of the executable, `tools/lib/sha256`). Verify: `tests/corpus/runner` host test (`corpus_runner`) with the pass file keys; `ctest --preset all` is unchanged for the 213 programs.
- [x] 2.2 Replace the `corpus_notes_*` and `corpus_originals_notes_*` ctests by tests of the new checks on `tests/corpus/selftest/` data: `corpus_expect_malformed_*` (no emulator, exit status 2 with the file name), `corpus_originals_fixed_regressed`, `corpus_originals_orig_changed`, `corpus_originals_exe_unlisted` (label `originals`, the program `t01_int`). Verify: each fails with its message on its broken input and passes on the correct one.

## 3. Targets

- [x] 3.1 `corpus-expect-update` refuses a changed unqualified file without `FIX=PASS:NAME#N` and turns it into the pair with it (`corpusrun pass --update-expect clib` extended; the pair writing in `corpusexpect`). Verify: host test; with the unchanged tree the target leaves `git status` clean; a fake run with a differing port output and no `FIX` names the program and writes nothing.
- [x] 3.2 Add `corpus-expect-record`: `ref` twice, the second with a Stack 16384 bytes larger (the layout of the bug-orig recording, `corpus_layout`; inject's `pad` changed what the bug-orig tests observe), the accept of 1.3, `FILTER`. Verify: `FILTER` on two programs leaves their files byte-identical; fake differing runs print "test not deterministic" and write nothing.
- [x] 3.3 `.exe` files in the record and update (`ref` and, with `FIX`, `port`). Verify: a fake pair is written and read back by the judge.

## 4. fix-scope

- [x] 4.1 `cmake/fix-scope.cmake` and `results fix-scope` project `tests/corpus/expect` (D5) and call `libscmp_fix_scope`. Verify: extend the `fix_scope_*` host tests: the corpus pair of `Fixes #6` passes; another changed file is reported with `corpus|t01_int|out|1`; a new program is reported added; a malformed directory fails. The `fix_scope_target` test gets a corpus case on a throwaway git repository.

## 5. Migration

- [x] 5.1 Convert the 14 rows of `notes.tsv` to files (D6): `.orig.out` from `corpus-expect-record` for `corpus_runerr:t124_bitmaperr`, `t126_rgberr` (#6), `t130_blit` (#46) and the programs of the library fixes #1000 to #1004 (`t141_shapetrix`, `t158_2dfuzz`, `t30_date`, `t121_palette`, `t21_val` in `corpus` and `corpus_runerr`), the `.exe` pair for `corpus:t130_blit` (and where else ref's and port's executables differ), the current expect files as `.fixed-N.out`; delete `tests/corpus/notes.tsv`. Verify: `ctest --preset all` and `ctest --preset originals` (CD) pass, `prev_results` byte-identical, `git status` after the originals run clean; `fix-scope` against the commit before reports only these programs as pairs of their issues (run with `--fixes` of each).
- [ ] 5.2 Record twice more (`corpus-expect-record` three times in a row, whole corpus) and check that no unqualified file changes. Verify: `git diff tests/corpus/expect` empty; any difference is a non-deterministic program: report it with its name before going on.

## 6. Acceptance

- [ ] 6.1 Full run: `ctest --preset all`, `ctest --preset originals` (CD), `ctest --preset host` with `-DHOST_SANITIZE=ON`, `ctest --preset bug-orig`; the `prev` comparison byte-identical for executables, TED files and libraries. Verify: all pass.

## 7. Documents

- [x] 7.1 `docs/testing.md`: suites table (corpus row: expected data `tests/corpus/expect`, a fix = the pair `NAME.orig.out` / `NAME.fixed-N.out`), how each configuration maps (the D2 table), remove the corpus from "How they differ" and "Planned follow-ups" and update the purpose line of `openspec/specs/testing/test-model/spec.md` ("bug-orig and the corpus follow") through the sync of this change. Verify: text agrees with the specs.
- [ ] 7.2 `tests/README.md` ("Corpus": the notes paragraph replaced by the pair rules, the targets `corpus-expect-update` with `FIX` and `corpus-expect-record`, the originals preset line), `tests/corpus/runner/README.md`, the header comment of `tests/corpus/CMakeLists.txt`, `docs/development.md`, `CLAUDE.md`, `.claude/agents/implementer.md` / `tester.md` where they name `notes.tsv`. Verify: `grep -rn "notes.tsv\|notes_pass" . --include=* ` (outside `archive/` and the archived OpenSpec changes) finds nothing current.
- [ ] 7.3 Site: `site/content/docs/project-structure/tests.md` and `site/content/docs/compared-with-original/comparing.md`. Verify: `hugo` via podman builds the pages and they were looked at.
- [ ] 7.4 `TODO.md`: the follow-up entry done (date). Verify: no strikethrough; `tools/testinfo/rules.txt` still selects the corpus tests for `tests/corpus/expect/**`.
