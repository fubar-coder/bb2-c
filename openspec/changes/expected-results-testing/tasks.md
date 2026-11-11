# Tasks

Groups 1-5 now (before further fixes); groups 6-10 per area, at the first
fix round of the area (design D8). Every group lands its tests and the
documentation it changes; no program code changes in this change.

## 1. Fixed bug-orig tests in `all`, and the new rules

- [x] 1.1 Runner: `; @fixed #NNN` header in `tests/corpus/runner/bugorig.c`/`bugorig.h` and `tests/bug-orig/CMakeLists.txt` (no `ref` config, verdict FIXED required, label `emulator` without `bug-orig`, ordering and `bug_orig_add_test` ignoring them). Verify: a test with the header runs in `ctest --preset all -N`, not in `--preset bug-orig -N`
- [x] 1.2 Add the header to the tests that come out FIXED today (the list of design Evidence 1, found by `ctest --preset bug-orig`: verdict FIXED). Verify: `ctest --preset all -R <each>` passes without the CD; `ctest --preset bug-orig -N` no longer lists them
- [x] 1.3 `bugorig_verdict` host test cases without `ref` (FIXED passes; REPRODUCED/DIFFERS/BROKEN impossible; a `@fixed` test showing the bug fails). Verify: the new cases pass and a deliberately wrong expectation fails
- [x] 1.4 Rewrite the rule "Do not fix original bugs" in CLAUDE.md (design D9), `.claude/agents/implementer.md` and `tester.md`, `tests/README.md` (presets table, bug-orig section, "Adding a test", "a fix turns its test into a fixed test in the same commit"), `docs/development.md`, `docs/libs/deviations.md` header rule. Verify: `grep -rn "Do not fix original bugs\|REPRODUCED with the reference" CLAUDE.md .claude tests/README.md docs` finds no current rule; every document names the same steps (spec `bug-fix-workflow`)
- [x] 1.5 TED bug-orig tests (`tests/bug-orig/ted/ID.drv`) get the fixed form `; @fixed #N` without a pair (pass key `fixed=1` instead of `orig=`, only C5, judged by the script's expectations alone: FIXED / REGRESSED naming the bug; no `.orig`; label `emulator` without `bug-orig`). Verify: `bug_orig_ted_fixed_selftest_fixed` (FIXED) and `bug_orig_ted_fixed_selftest_regressed` (REGRESSED) in the emulator, the refusals (`bug_orig_ted_fixed_*`, `bug_orig_ted_open_fixed_script`, `bug_orig_ted_fixed_wrong_lint`), the host case "fixed bug-orig test" of `test_tedab`; an existing REPRODUCED script marked `@fixed` comes out REGRESSED in `ctest --preset all` and leaves `--preset bug-orig`

## 2. One expect suite for Blitz-program tests

- [x] 2.1 `tests/blitz2` drops the CD-libs configuration (keep `clib`); the port regression tests of `tests/bug-orig/regression/` use the same runner and file format without `ref`. Verify: `ctest --preset all -R 'blitz2_|regression_'` passes with `archive/cd` absent
- [x] 2.2 Document the suite in `tests/README.md` ("Extensions of blitz2", "Port regression tests") and the fixed-test rule for it. Verify: the documented command runs as written

## 3. Intended lists (format now, entries at the first fix)

- [x] 3.1 `rwe_declarations_NAME` passes `libs/userlibs/rwe/NAME.declarations-intended.txt` to `blitzlib compare` when present (`libs/userlibs/rwe/CMakeLists.txt:201`). Verify: a host test with a stale and a missing entry fails, a correct one passes
- [x] 3.2 `checkdata` accepts `programs/ted/checkdata-intended.txt` (entry, issue; each must occur). Verify: `tools/tests` cases with a stale entry, an unlisted difference and a correct list
- [x] 3.3 `asmcmp` rule documented in `libs/acidlibs/CMakeLists.txt` comment and `docs/development.md`: a fix removes its routine from the list in the same commit. Verify: the text is present; no test change
- [x] 3.4 Document the three formats in `tests/README.md` and `docs/development.md` (checks section). Verify: the documented example entries are accepted by the tools

## 4. `expect_fixed` in `libs_emulator`

- [x] 4.1 `libs/tests/sysload.h`: `expect_fixed(NNN, value)` replaces the equality with `orig` for one check and fails when the C library gives `orig`'s value. Verify: snitch cases with a fixed value (passes), a wrong value (fails), a stale exemption (fails), run in the emulator
- [x] 4.2 Document the helper and the rule "no scenario dodging" in `tests/README.md` (Libraries). Verify: the example in the text compiles in a test case

## 5. `ted_ab` directive `; @fixed #NNN PAIR...`

- [x] 5.1 `tests/ted-ab` runner: the named pairs (for example `R5-C5 Rc-Cc`) must differ, the rest of the comparison stays; BROKEN if they do not differ. Verify: `ted_ab_lint` and a host test with a script whose named pair does not differ
- [x] 5.2 Document the directive in `tests/README.md` (TED A/B). Verify: a sample script of the text passes `ted_ab_lint`

## 6. Recorded original observations (`ID.orig`)

- [x] 6.1 Runner: recording mode (`BUGORIG_RECORD=1`, ctest `bug_orig_record_ID` under label `originals`) writing `ID.orig`; the normal run judges `port`/`clib` against `ID.expect` and `ID.orig`, starts no `ref` session; BROKEN checked at parse time. Verify: `bugorig_verdict` host cases for REPRODUCED, DIFFERS, BROKEN without `ref`; `bug_orig_selftest_broken` works on a recorded file
- [x] 6.2 TED bug-orig tests (`tedab` bugorig mode): record the R5 observation the same way. Verify: `bug_orig_ted_selftest_broken` on the recorded file; the 22 TED tests give the same verdicts as with R5 live
- [x] 6.3 One recording run with the CD for all 350 tests, run twice with identical files, reviewed (wildcards for nondeterministic bits), committed. Verify: `ctest --preset bug-orig` gives the verdicts it gave with live `ref` (compare the lists) with no `ref` session started, also with `archive/cd` absent except tests that mount BB2
- [x] 6.4 Document recording and the `.orig` files in `tests/README.md` (bug-orig sections, "Adding a test": `ID.orig` replaces "look at the observation of ref") and `docs/development.md`. Verify: adding a test by the text yields REPRODUCED
- [x] 6.5 A fixed bug-orig test (`; @fixed #N` in `.bas`, `@fixed` in a TED `.drv`) keeps no `ID.orig`: configure-time check `tests/bug-orig/check-fixed-orig.cmake`. Verify: ctest `bug_orig_fixed_orig_broken` names the stale files of the fake tree `tests/bug-orig/selftest/fixed-orig/`

## 7. Corpus expect files, CD table, TED oracle

- [x] 7.1 Runner mode `expect_dir=` in `tests/corpus/runner/corpus.h`; tests of the runner with inputs from memory (matching output, differing output, missing file). Verify: host tests pass, broken inputs fail
- [x] 7.2 Target `corpus-expect-update` and the generated `tests/corpus/expect/NAME.out` (~200 files, serial bytes for the serial tests), reviewed by `git diff` for outputs that are original bugs. Verify: `ctest --preset all -R corpus` passes in every config
- [x] 7.3 Target `blitz2-cd-update`: rewrites `tests/data/blitz2-cd.tsv` from the port's result, keeping note rows (`#NNN`, the CD's hash or error line); the generator keeps them. Verify: a run on the unchanged tree changes no byte; `tools/tests` cases for a kept and a dropped note
- [x] 7.4 TED oracle: note column `#NNN` in `tests/data/ted-oracle.sha256` and the generator keeping such rows (`programs/ted/tests/oracle`). Verify: `ted_core_*` pass; a regenerated file equals the committed one when no fix is present; a tool test for a kept row
- [x] 7.5 Document in `tests/README.md` (Corpus, Oracle data of the TED tests) and `docs/development.md`. Verify: the commands in the text run

## 8. The `prev` build

- [x] 8.1 `BLITZ2_PREV` option, `git archive` + ExternalProject of the Amiga side, results in `build/<preset>/prev/blitz2/` (`CMakeLists.txt`). Verify: configure without the option makes no second build; with it `prev/blitz2/` holds the previous commit's results and the checkout is untouched
- [x] 8.2 Host test `prev_results` (library files, `acidlibs`, `deflibs` byte by byte; `-DPREV_INTENDED=ON` or list; 77 without a prev build). Verify: passes on a rework commit; fails when one library file is changed; skipped without prev
- [x] 8.3 Corpus config `prev|exe|...` and `ted_ab` combos `P5`/`Pc` with `Cc-Pc` comparison. Verify: `ctest --preset all -R 'corpus|ted_ab'` passes with a prev build; a change of generated code in a test commit shows in the `prev` comparison
- [x] 8.4 Document in `tests/README.md`, `docs/development.md` and CLAUDE.md "Acceptance" (the manual worktree comparison is replaced). Verify: the documented command runs as written

- [x] 8.5 `PREV_INTENDED` also names corpus sources and `ted_ab` cases (`corpus:NAME`, `corpus_runerr:NAME`, ..., `ted_ab:CASE[:PAIR]`) with the rule of the library files (a listed entry must differ, an unlisted difference fails, `ON` allows all); parsing and matching in `tools/lib/previntended`; the reports end with the list to quote. Verify: `test_previntended`, `corpus_runner`, `ted_ab_tedab`; with a test commit that changes generated code all corpus sources differ, one listed source passes while the others fail, all listed pass, a listed source that does not differ fails as stale

## 9. Label `originals`

- [x] 9.1 Label and preset `originals` for the tests of spec `original-comparisons`; `oracle` unchanged (`CMakePresets.json`, `CMakeLists.txt`, the test definitions). Verify: `ctest --preset originals -N` lists exactly those; `ctest --preset all` without `archive/cd` skips nothing for a missing reference
  - Corpus part done (2026-11-09; design.md "Task 9.1, the corpus part"): the live `ref` and `port` configurations of `corpus`, `corpus_runerr`, `corpus_req`, `corpus_serial` and `corpus_serial_runerr` are the tests `NAME_originals` (label `originals`) and `tests/corpus/notes.tsv` lists the intended deviations. Not done: `ted_ab` (its fixture is `corpus_originals` meanwhile, so `ted_ab` still pulls that test into `all`), `rwe_refbuild*`, `rwe_residents_cd`, `rwe_lesfont_cd`, `tokens_cd`, the CD result table and TED oracle generators, `corpus_cd`/`corpus_cd_ab3`.
- [x] 9.2 A1/A2 (reference builds of blitz2 and TED) become opt-in targets. Verify: a default build does not run vasm of the originals; `ctest --preset originals` builds them on demand
  - blitz2's reference build and `tedoracle` done (2026-11-11). TED's (A2): decision of 2026-11-11: the reference data `checkdata` reads (`ref/ted.lst`, `ref/ted`, as far as `programs/ted/checkdata.tsv` needs them) are committed as generated data under `tests/data/`; `checkdata` stays a check in the build step of `ted.sym` and reads them; a test of `originals` compares them with a fresh reference build of `ted.s`, with a broken-input test; the target that writes them is documented
- [x] 9.3 After `test-coverage` is archived: delta `testing/test-selection` (`.orig` files and `tests/corpus/expect/` are inputs of the selection rules, `tools/covsel/rules.txt`). Verify: `openspec validate` passes; a sample diff of an `.orig` file selects its bug-orig test
- [x] 9.4 Document the label and presets in `tests/README.md` (presets table) and CLAUDE.md "Building and checking". Verify: the table lists the same presets as `ctest --list-presets`
- [x] 9.5 A configuration `orig-clib` in the `*_originals` corpus tests: the original blitz2 2.10 with the build's acidlibs and deflibs; output judged against the expected files like `clib`, intended deviations in `tests/corpus/notes.tsv`. Verify: `ctest --preset originals` runs it for every corpus pass; a program that differs and is not listed fails
- [x] 9.6 Library declarations as data: the declarations of the original libraries that `declarations_acidlibs`, `declarations_deflibs` and `rwe_declarations_NAME` compare with are committed under `tests/data/` (as `tests/data/ted-tokens.txt`); the tests in `all` read them; a test of `originals` compares the data with the CD. Verify: `ctest --preset all` without `archive/cd` runs these tests; a changed CD file is reported by `originals`
- [x] 9.7 `asmcmp_*` (instructions of CD routines) move to `originals`: a dump of the routines would be a copy of the CD's code. Verify: `ctest --preset all -N` lists none
- [x] 9.8 `libs_emulator` against recorded results: the results of the original libraries (return values, registers, memory of each check) are recorded once into committed files like the `.orig` files; the test in `all` compares the C libraries with them, intended differences with `expect_fixed`; recording and the comparison with the live original under `originals`. Verify: all without the CD runs it; a recording run gives the committed files
- [x] 9.9 The fixed TED bug-orig tests (`; @fixed #N`) run in `all` with combination Cc (blitz2-c); C5 (the original compiler) under `originals`. Verify: all without the CD runs them; a REGRESSED script fails in both
- [x] 9.10 The passes with runtime error checking do not need the CD's `DEFAULTDBUG` (the corpus builds executables, which link runerrlib + rundebuglib); the pass files of `all` name no `defaultdbug=`, and a missing module never skips a pass. Verify: `ctest --preset all` with and without `archive/cd` runs corpus_runerr, corpus_req and corpus_serial_runerr with the committed expect files
- [x] 9.11 `bug_orig_compiler_loaddmod_pointer` and `ab3_bb2_equivalence*`: what of the CD they read, then recorded data or `originals`. Verify: all without the CD skips no test for a missing reference (the spec scenario)

## 10. Close-out

- [x] 10.1 `README.md` ("State"), `docs/libs/deviations.md` (kind F points to the test kinds), TODO.md section 3 entry. Verify: no document says original bugs are not fixed
- [ ] 10.2 Integration check: `ctest --preset all` without the CD, `ctest --preset originals` with it, `ctest --preset bug-orig`, `ctest --preset host` with `-DHOST_SANITIZE=ON`; results of the programs byte-identical to the `prev` build. Verify: the runs pass or fail only where documented

## Workflow follow-up

- Archive the change after the review; the deltas of `testing/*` create the main specs, `ab3-compat/bb2-equivalence` is modified.
