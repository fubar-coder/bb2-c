# Tasks

Implementation starts after compiler fix round R3 is merged (group 0).
One commit per numbered group from 1 on; a group is completely switched
over in its commit (design D9). The common checklist applies to groups
2-11 and is not repeated there:

- **C1** the program: `main.c` with the subcommand table, one
  `cmd_NAME.c` per former `main.c` (moved, not rewritten), the old tool
  directories removed, test scripts moved with the program;
- **C2** every call site (counts in design D10): CMake variables and
  `BUILD_BYPRODUCTS` in `CMakeLists.txt`, `add_test`/`add_custom_command`,
  `*.cmake` scripts, paths in `cmake/*.cmake`, `tools/covsel/rules.txt`;
- **C3** `help_PROGRAM` with the subcommand list, `readme_PROGRAM`, the
  program's `README.md`;
- **C4** `docs/structure.md`, `tests/README.md`, `docs/development.md`
  link the README instead of describing the tool; old and new calls in the
  commit message;
- **C5** verification (design D9): clean build without warnings, also
  `-DHOST_SANITIZE=ON`; `-DBLITZ2_PREV=ON` with `prev_results`, corpus and
  `ted_ab` prev parts, no `PREV_INTENDED`; `ctest --preset host` and
  `ctest --preset all`; a search for the old names as command, variable
  and path finds nothing; each subcommand run before/after on the same
  input with `cmp`.

## 0. Preconditions

- [x] 0.1 Check that compiler fix round R3 is merged into `main`: every R3 issue of `~/.local/share/bb2-c-work/compiler-bugs-inventory.md` is closed or its fix commit is an ancestor of `main` (`git merge-base --is-ancestor`), and the coordinator confirms. Verify: the list of R3 commits is in the first commit message of group 1; if R3 is not merged, stop
- [x] 0.2 Baseline: on `main`, clean build, `ctest --preset all` and `ctest --preset host` pass; record the failing set (if any) in `~/.local/share/bb2-c-work/tools-baseline.md`. Verify: the file exists; later groups compare against it
- [x] 0.3 Settle open questions 1-5 of design.md with the user (hunkcheck, `--purge-snapshots`, exit codes, `testinfo`, one or two runners); update design D1 and the table if an answer changes the grouping. Verify: the design has no open question left that a task depends on
- [x] 0.4 Agree with the coordinator that no other change touches the tool directories while a group is in progress (merge conflicts in 20 files per group). Verify: a note in the group's report

## 1. Infrastructure

- [x] 1.1 `tools/lib/subcmd/` (`subcmd.h`, `subcmd.c`, `lib_subcmd`) as in design D3 and `tools/tests/test_subcmd.c` with inputs in memory: no argument, unknown name, `-h`, `--help`, `SUB -h`, `SUB --help`, a subcommand that returns 1, `argv[0]` of the subcommand is `PROGRAM SUB`. Verify: `ctest --preset host -R subcmd` passes; a deliberately wrong table (a subcommand without usage) fails the test
- [x] 1.2 `cmake/check-help.cmake`: `SUBCOMMANDS`, per-subcommand help, the synopsis lines of `-h` against the list (design D5); `host_help_test()` in `host/CMakeLists.txt` replaces the loop at lines 50-54 (all existing programs keep their `help_NAME` tests, single-mode, until their group). Verify: all existing `help_*` pass; a test with a fake program whose subcommand help writes to stderr, and one whose table has a subcommand the list lacks, fails as `WILL_FAIL`
- [x] 1.3 `cmake/check-readme.cmake` and `readme_PROGRAM` tests (design D6), with a fake program and a README without one synopsis line as `WILL_FAIL`. Verify: both broken cases fail, a correct pair passes
- [x] 1.4 README template and the convention (syntax, exit codes, message form, README sections, adding a subcommand) in `docs/development.md` under the tools / checks section. Verify: the example in the text runs as written; one existing tool's README written from the template (`tools/hdftool/README.md`, no code change) passes `readme_hdftool`
- [x] 1.5 Commit "tools: subcmd dispatch library, help and README checks". Verify: C5 of the checklist, all results unchanged (no program changed)

## 2. `testinfo` (teststats, covmap, covsel; 7 call sites)

- [x] 2.1 `tools/testinfo/` with `stats`, `map`, `select` (`teststats`, `covmap`, `covsel`); `cmake/coverage-map.cmake:29`, `cmake/coverage-select.cmake:30` call `testinfo map` and `testinfo select`; `rules.txt:57-61` patterns become `tools/testinfo/**`. Verify: C1-C5; `cmake -P cmake/coverage-map.cmake` and `coverage-select.cmake` on the host coverage build produce the same files as before (`cmp` of `lcov/*.info`, `selected`); `test_covmap`, `test_covsel`, `test_teststats` unchanged and passing

## 3. `refcheck` (asmcmp, checklabels, checkdata, vasmlisting; 22 call sites)

- [x] 3.1 `tools/refcheck/` with `asm`, `labels`, `data`, `listing-map`, `listing-labels`, `listing-verify`; the argument order of `vasmlisting LISTING map EXE` becomes `refcheck listing-map LISTING`, `listing-verify LISTING EXE`. Call sites: `libs/acidlibs/CMakeLists.txt`, `programs/ted/CMakeLists.txt` (9), `programs/blitz2/CMakeLists.txt` (4), `CMakeLists.txt`. Verify: C1-C5; the tests that run the four tools (`ctest -N` lists them by the callers `libs/acidlibs`, `programs/ted`, `programs/blitz2`) pass with the same reports (`cmp`); an intended list (`programs/ted/checkdata-intended.txt`, `INTENDED` of `blitzlibcompare`) is still read, checked with a stale entry

## 4. `results` (outmatch, prevcmp, cdtable; 25 call sites)

- [x] 4.1 `tools/results/` with `match`, `prev`, `cd-update`, `cd-reference`. Call sites: `tests/ab3`, `tests/bug-orig` (`outmatch`, including `inject-*.cmake`), `CMakeLists.txt` (`prev_results`, the `prevcmp_*` tests of `tools/tests`), `tests/corpus/CMakeLists.txt` (`cdtable`). `rules.txt:55-56` patterns follow (`tools/results/**` keeps `label:emulator`). Verify: C1-C5; `prev_results` passes, exit 77 without a prev build is unchanged; the targets `blitz2-cd-reference` and `blitz2-cd-update` (`tests/corpus/CMakeLists.txt:179,191`) produce the same table (`cmp`)

## 5. `hunkcheck` (10 call sites)

- [x] 5.1 `tools/hunkcheck/` with `start`, `layout`, `stored` (unless open question 1 says otherwise). Call sites: `programs/ab3cli`, `programs/ted`, `programs/ted/tests/oracle`, `tests/ab3`, `tests/corpus/amiga`, `CMakeLists.txt`. Verify: C1-C5; every POST_BUILD check still runs (break a hunk layout on purpose in a scratch copy and see the build fail)

## 6. `blitzres` (12 call sites)

- [x] 6.1 `blitzres create|dump|source` instead of `-o OUT ITEM...`, `--dump`, `--source`; call sites `libs/userlibs/rwe/residents.cmake` (6), `residents-cd.cmake`, `tools/blitzres/check.cmake`, `CMakeLists.txt`; README. Verify: C1-C5; the residents of the RWE reference build are byte-identical (`cmp` of the files `residents.cmake` writes), the tests calling `blitzres` (`tools/blitzres/check.cmake`) pass

## 7. `blitzlib` (biglib, blitzlibcheck, blitzlibcompare, cmdref; 31 call sites)

- [x] 7.1 `tools/blitzlib/` with `join`, `extract`, `numbers`, `check`, `compare`, `cmdref`. Call sites: `cmake/BlitzLibrary.cmake` (6, used by every library target), `libs/CMakeLists.txt`, `libs/tests/CMakeLists.txt`, `libs/tests/emulator.cmake`, `libs/userlibs/rwe/CMakeLists.txt` and `refbuild.cmake`, `tools/cmdref/check.cmake`, `complete.cmake`, `CMakeLists.txt`. Verify: C1-C5; **`acidlibs`, `deflibs`, every `.obj` and the site JSON `site/data/libraries/` are byte-identical** (`prev_results` and `cmp` of a regenerated `cmdref` output); `rwe_numbers*`, `biglib_check_clash` and `tokens_*` tests pass (their names stay)

## 8. `tedab` (tedab, teddump; 10 call sites)

- [x] 8.1 `tests/ted-ab/runner/`: `tedab run|lint|info|text|cmp`; `main_teddump.c` becomes the body of the last three, `TED_FONTS_DIR` moves to the `tedab` target; call sites `tests/ted-ab/CMakeLists.txt` (4), `tests/bug-orig/CMakeLists.txt` (5), `CMakeLists.txt`; README in the runner directory. Verify: C1-C5; the `ted_ab` pass (needs Amiberry) gives the same verdicts and `cmp` of a screen dump text of `teddump text` and `tedab text`

## 9. `corpusrun` (corpusrun, cdrun; 22 call sites)

- [x] 9.1 `tests/corpus/runner/`: `corpusrun pass|cd PASSFILE`; `cdrun.c`'s `main` becomes the subcommand function (`CDRUN_FORCE`, `BUGORIG_RECORD` stay); `check-cdrun-source.cmake`, `tests/corpus/CMakeLists.txt` (5), `tests/bug-orig/CMakeLists.txt` (5) and `want.cmake`, `tests/ab3`, `tests/blitz2`, `libs/userlibs/rwe/residents*.cmake` and `refbuild.cmake`, `CMakeLists.txt`; README in the runner directory. Verify: C1-C5; the corpus passes with the same verdicts, `cdrun_source`, `cdrun_cache`, `corpus_runner`, `bugorig_verdict` pass, the table of `blitz2-cd-reference` is identical

## 10. `tedsrc` (tedtok, teddetok; 45 call sites)

- [x] 10.1 `programs/ted/tools/tedsrc/` with `tok` and `detok` (`--check` stays an option), linked with `tedhost` and `tedcore_host` as before; `host/CMakeLists.txt:39-43` adds the one directory; call sites `libs/userlibs/rwe` (`refbuild*.cmake`, `residents.cmake` (4), `CMakeLists.txt`), `tests/ab3`, `tests/blitz2`, `tests/bug-orig` (4), `tests/corpus`, `tests/ted-ab`, `tools/bb22/rwelibsrc.cmake`, `programs/ted/tools/*/tests/*.cmake`, `CMakeLists.txt`; the tests `tedtok_*`, `teddetok_*` stay by name. Verify: C1-C5; `teddetok_bblibsrc` (needs `archive/aminet`) and `tedtok_libs` (needs the CD) are run, not skipped; the token files and tokenised sources of the corpus are byte-identical (`cmp`)

## 11. `bbsrc` (blitztokens, bbdetok, bblines, bb22, ab3tok; 56 call sites)

- [x] 11.1 `tools/bbsrc/` with `tokens`, `detok`, `comment`, `bb22`, `bb22-info`, `ab3`, `ab3-cmp`; the test scripts of the five directories move (`tools/bbdetok/tooltest.cmake` has 19 calls, `archive_text.cmake`; `tools/ab3tok/tooltest.cmake`; `tools/bb22/rwelibsrc.cmake`; `tools/blitztokens/check.cmake`), messages they match get the `bbsrc SUB:` prefix; call sites `libs/tests`, `libs/userlibs/rwe` (`bb22`, `bblines`), `tests/ab3` (`tokenise.cmake`, `tokens.cmake`, `CMakeLists.txt`), `tests/bug-orig` (`blitztokens`), `CMakeLists.txt`. Verify: C1-C5; `tokens_*` lists and `tests/data/ted-tokens.txt` regenerate byte-identical, the tests of `tests/ab3` that tokenise pass, `ctest --preset oracle` passes

## 12. READMEs of the programs without code change, and the end

- [ ] 12.1 `README.md` for `tools/amiga_run`, `tools/mktestdata`, `tools/xtra`, `tools/hdftool` (hdftool's from 1.4 reviewed), each with a `readme_PROGRAM` test; `tools/amiga_run/README.md` links `tools/lib/amiga_emulation/README.md`; `help_*` for the programs with subcommands (`xtra`, `hdftool`) run them. Verify: `readme_*` and `help_*` pass
- [ ] 12.2 Final sweep: `docs/structure.md` (tools section), `tests/README.md`, `docs/development.md`, `README.md`, `TODO.md` (the entry "-h / --help for all callable programs" names the new programs; a done entry for this change with the commits) name only the new programs and link the READMEs; every host program has a README. Verify: a search for the 31 former names as command, variable or path finds nothing outside `archive/`, `TODO.md`, `openspec/`; `ls tools/*/README.md programs/ted/tools/*/README.md tests/*/runner/README.md` covers all 14 programs
- [ ] 12.3 Whole-state verification on the last commit versus the commit before group 1: `-DBLITZ2_PREV=<that commit>`, `prev_results`, corpus and `ted_ab` prev parts, `ctest --preset all`, `ctest --preset oracle`, a `-DHOST_SANITIZE=ON` host run. Verify: no difference in any result; failures equal the baseline of 0.2 or fewer
- [ ] 12.4 Archive the change (`openspec archive host-tools-consolidation`) after the coordinator accepted the commits. Verify: `openspec/specs/tooling/host-tools/spec.md` exists
