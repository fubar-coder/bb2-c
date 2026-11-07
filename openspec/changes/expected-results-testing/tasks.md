# Tasks

Groups 1-5 now (before further fixes); groups 6-10 per area, at the first
fix round of the area (design D8). Every group lands its tests and the
documentation it changes; no program code changes in this change.

## 1. Fixed bug-orig tests in `all`, and the new rules

- [x] 1.1 Runner: `; @fixed #NNN` header in `tests/corpus/runner/bugorig.c`/`bugorig.h` and `tests/bug-orig/CMakeLists.txt` (no `ref` config, verdict FIXED required, label `emulator` without `bug-orig`, ordering and `bug_orig_add_test` ignoring them). Verify: a test with the header runs in `ctest --preset all -N`, not in `--preset bug-orig -N`
- [x] 1.2 Add the header to the tests that come out FIXED today (the list of design Evidence 1, found by `ctest --preset bug-orig`: verdict FIXED). Verify: `ctest --preset all -R <each>` passes without the CD; `ctest --preset bug-orig -N` no longer lists them
- [x] 1.3 `bugorig_verdict` host test cases without `ref` (FIXED passes; REPRODUCED/DIFFERS/BROKEN impossible; a `@fixed` test showing the bug fails). Verify: the new cases pass and a deliberately wrong expectation fails
- [ ] 1.4 Rewrite the rule "Do not fix original bugs" in CLAUDE.md (design D9), `.claude/agents/implementer.md` and `tester.md`, `tests/README.md` (presets table, bug-orig section, "Adding a test", "a fix turns its test into a fixed test in the same commit"), `docs/development.md`, `docs/libs/deviations.md` header rule. Verify: `grep -rn "Do not fix original bugs\|REPRODUCED with the reference" CLAUDE.md .claude tests/README.md docs` finds no current rule; every document names the same steps (spec `bug-fix-workflow`)

## 2. One expect suite for Blitz-program tests

- [ ] 2.1 `tests/blitz2` drops the CD-libs configuration (keep `clib`); the port regression tests of `tests/bug-orig/regression/` use the same runner and file format without `ref`. Verify: `ctest --preset all -R 'blitz2_|regression_'` passes with `archive/cd` absent
- [ ] 2.2 Document the suite in `tests/README.md` ("Extensions of blitz2", "Port regression tests") and the fixed-test rule for it. Verify: the documented command runs as written

## 3. Intended lists (format now, entries at the first fix)

- [ ] 3.1 `rwe_declarations_NAME` passes `libs/userlibs/rwe/NAME.declarations-intended.txt` to `blitzlibcompare` when present (`libs/userlibs/rwe/CMakeLists.txt:201`). Verify: a host test with a stale and a missing entry fails, a correct one passes
- [ ] 3.2 `checkdata` accepts `programs/ted/checkdata-intended.txt` (entry, issue; each must occur). Verify: `tools/tests` cases with a stale entry, an unlisted difference and a correct list
- [ ] 3.3 `asmcmp` rule documented in `libs/acidlibs/CMakeLists.txt` comment and `docs/development.md`: a fix removes its routine from the list in the same commit. Verify: the text is present; no test change
- [ ] 3.4 Document the three formats in `tests/README.md` and `docs/development.md` (checks section). Verify: the documented example entries are accepted by the tools

## 4. `expect_fixed` in `libs_emulator`

- [ ] 4.1 `libs/tests/sysload.h`: `expect_fixed(NNN, value)` replaces the equality with `orig` for one check and fails when the C library gives `orig`'s value. Verify: snitch cases with a fixed value (passes), a wrong value (fails), a stale exemption (fails), run in the emulator
- [ ] 4.2 Document the helper and the rule "no scenario dodging" in `tests/README.md` (Libraries). Verify: the example in the text compiles in a test case

## 5. `ted_ab` directive `; @fixed #NNN PAIR...`

- [ ] 5.1 `tests/ted-ab` runner: the named pairs (for example `R5-C5 Rc-Cc`) must differ, the rest of the comparison stays; BROKEN if they do not differ. Verify: `ted_ab_lint` and a host test with a script whose named pair does not differ
- [ ] 5.2 Document the directive in `tests/README.md` (TED A/B). Verify: a sample script of the text passes `ted_ab_lint`

## 6. Recorded original observations (`ID.orig`)

- [ ] 6.1 Runner: recording mode (`BUGORIG_RECORD=1`, ctest `bug_orig_record_ID` under label `originals`) writing `ID.orig`; the normal run judges `port`/`clib` against `ID.expect` and `ID.orig`, starts no `ref` session; BROKEN checked at parse time. Verify: `bugorig_verdict` host cases for REPRODUCED, DIFFERS, BROKEN without `ref`; `bug_orig_selftest_broken` works on a recorded file
- [ ] 6.2 TED bug-orig tests (`tedab` bugorig mode): record the R5 observation the same way. Verify: `bug_orig_ted_selftest_broken` on the recorded file; the 22 TED tests give the same verdicts as with R5 live
- [ ] 6.3 One recording run with the CD for all 350 tests, run twice with identical files, reviewed (wildcards for nondeterministic bits), committed. Verify: `ctest --preset bug-orig` gives the verdicts it gave with live `ref` (compare the lists) with no `ref` session started, also with `archive/cd` absent except tests that mount BB2
- [ ] 6.4 Document recording and the `.orig` files in `tests/README.md` (bug-orig sections, "Adding a test": `ID.orig` replaces "look at the observation of ref") and `docs/development.md`. Verify: adding a test by the text yields REPRODUCED

## 7. Corpus expect files, CD table, TED oracle

- [ ] 7.1 Runner mode `expect_dir=` in `tests/corpus/runner/corpus.h`; tests of the runner with inputs from memory (matching output, differing output, missing file). Verify: host tests pass, broken inputs fail
- [ ] 7.2 Target `corpus-expect-update` and the generated `tests/corpus/expect/NAME.out` (~200 files, serial bytes for the serial tests), reviewed by `git diff` for outputs that are original bugs. Verify: `ctest --preset all -R corpus` passes in every config
- [ ] 7.3 Target `blitz2-cd-update`: rewrites `tests/data/blitz2-cd.tsv` from the port's result, keeping note rows (`#NNN`, the CD's hash); the generator keeps them. Verify: a run on the unchanged tree changes no byte; `tools/tests` cases for a kept and a dropped note
- [ ] 7.4 TED oracle: note column `#NNN` in `tests/data/ted-oracle.sha256` and the generator keeping such rows (`programs/ted/tests/oracle`). Verify: `ted_core_*` pass; a regenerated file equals the committed one when no fix is present; a tool test for a kept row
- [ ] 7.5 Document in `tests/README.md` (Corpus, Oracle data of the TED tests) and `docs/development.md`. Verify: the commands in the text run

## 8. The `prev` build

- [ ] 8.1 `BLITZ2_PREV` option, `git archive` + ExternalProject of the Amiga side, results in `build/<preset>/prev/blitz2/` (`CMakeLists.txt`). Verify: configure without the option makes no second build; with it `prev/blitz2/` holds the previous commit's results and the checkout is untouched
- [ ] 8.2 Host test `prev_results` (library files, `acidlibs`, `deflibs` byte by byte; `-DPREV_INTENDED=ON` or list; 77 without a prev build). Verify: passes on a rework commit; fails when one library file is changed; skipped without prev
- [ ] 8.3 Corpus config `prev|exe|...` and `ted_ab` combos `P5`/`Pc` with `Cc-Pc` comparison. Verify: `ctest --preset all -R 'corpus|ted_ab'` passes with a prev build; a change of generated code in a test commit shows in the `prev` comparison
- [ ] 8.4 Document in `tests/README.md`, `docs/development.md` and CLAUDE.md "Acceptance" (the manual worktree comparison is replaced). Verify: the documented command runs as written

## 9. Label `originals`

- [ ] 9.1 Label and preset `originals` for the tests of spec `original-comparisons`; `oracle` unchanged (`CMakePresets.json`, `CMakeLists.txt`, the test definitions). Verify: `ctest --preset originals -N` lists exactly those; `ctest --preset all` without `archive/cd` skips nothing for a missing reference
- [ ] 9.2 A1/A2 (reference builds of blitz2 and TED) become opt-in targets. Verify: a default build does not run vasm of the originals; `ctest --preset originals` builds them on demand
- [ ] 9.3 After `test-coverage` is archived: delta `testing/test-selection` (`.orig` files and `tests/corpus/expect/` are inputs of the selection rules, `tools/covsel/rules.txt`). Verify: `openspec validate` passes; a sample diff of an `.orig` file selects its bug-orig test
- [ ] 9.4 Document the label and presets in `tests/README.md` (presets table) and CLAUDE.md "Building and checking". Verify: the table lists the same presets as `ctest --list-presets`

## 10. Close-out

- [ ] 10.1 `README.md` ("State"), `docs/libs/deviations.md` (kind F points to the test kinds), TODO.md section 3 entry. Verify: no document says original bugs are not fixed
- [ ] 10.2 Integration check: `ctest --preset all` without the CD, `ctest --preset originals` with it, `ctest --preset bug-orig`, `ctest --preset host` with `-DHOST_SANITIZE=ON`; results of the programs byte-identical to the `prev` build. Verify: the runs pass or fail only where documented

## Workflow follow-up

- Archive the change after the review; the deltas of `testing/*` create the main specs, `ab3-compat/bb2-equivalence` is modified.
