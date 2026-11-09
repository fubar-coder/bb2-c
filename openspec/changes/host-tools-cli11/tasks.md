# Tasks

One commit per numbered group from 1 on. The common checklist applies to
groups 2-14 and is not repeated there:

- **C1** the program: `main.cpp` with the subcommand declarations and
  `cli::run`, one `cmd_NAME.cpp` per subcommand (C library headers inside
  `extern "C"`), no `argv` loop, the old `main.c`/`cmd_*.c` removed (git
  moves, not copies), the program's CMake target `CXX_STANDARD 20`;
- **C2** every caller of the program: options that were renamed, calls
  that pass a value starting with `-`, `*.cmake` scripts, tests that match
  messages (`PASS_REGULAR_EXPRESSION`, `MATCHES`) or exit codes (design D7
  table); the exit codes in the README follow the code;
- **C3** `help_PROGRAM` and `readme_PROGRAM` use the new check scripts
  (design D6), the program's `README.md` has the verbatim help blocks and
  documents the exit codes;
- **C4** `docs/structure.md`, `tests/README.md`, `docs/development.md`
  name the program as before and link the README; old and new calls in
  the commit message;
- **C5** verification: clean build without warnings, also
  `-DHOST_SANITIZE=ON`; `ctest --preset host` and the tests of the
  program; `-DBLITZ2_PREV=ON` with `prev_results`, corpus and `ted_ab`
  prev parts, no `PREV_INTENDED`; each subcommand run on the same input
  before (the commit before) and after, outputs and stdout compared with
  `cmp`, apart from help and error messages; usage errors give 2.

## 0. Preconditions

- [ ] 0.1 Baseline: on `main`, clean build, `ctest --preset all` and `ctest --preset host` pass; record the failing set (if any) in `~/.local/share/bb2-c-work/cli11-baseline.md`. Verify: the file exists; later groups compare against it
- [ ] 0.2 Settle the open questions of design.md that change what is built (1 version, 4 renames, 6 verdict codes) with the coordinator; note the answers in design.md. Verify: no open question is left that a task depends on
- [ ] 0.3 Agree with the coordinator that no other change touches the tool directories while a group is in progress. Verify: a note in the group's report

## 1. Infrastructure

- [ ] 1.1 `cmake/CLI11.cmake` (pinned release tarball, SHA256 computed from the download, `EXCLUDE_FROM_ALL SYSTEM`, tests/examples/install off), included from `host/CMakeLists.txt`; `cmake/prev.cmake` and `cmake/prev-build.cmake` pass the CLI11 pin and source on (design D1, D2). Verify: a host configure downloads once and a second build directory with `-DFETCHCONTENT_SOURCE_DIR_CLI11` does not; `cmake --preset amiga -DBLITZ2_PREV=ON` configures the prev build with the network switched off (`unshare -rn`)
- [ ] 1.2 `tools/lib/cli/cli.hpp` (design D3: `cli::run`, formatter, parse errors to 2) and `tools/tests/test_cli.cpp`: help of the program and of a subcommand (0, stdout), unknown subcommand, unknown option, missing value, surplus argument (2, stderr), a subcommand's status 1 and 77 passed on, a `std::exception` from a validator (2). Verify: `ctest --preset host -R cli` passes; a deliberately broken `cli::run` (returning CLI11's own code) fails it
- [ ] 1.3 `cmake/check-help-cli.cmake` and `cmake/check-readme-cli.cmake` (design D6), the fake program `tools/tests/fake_cli11.cpp` (alpha, beta; `fake_cli11_broken` with a stderr message in `beta -h`) and fake READMEs `ok`, `missing`, `changed`, `stale`; `host_help_test()`/`host_readme_test()` take the script name. Verify: `help_check_ok`, `readme_check_ok` pass; `help_check_stderr`, `help_check_unlisted`, `help_check_missing`, `readme_check_missing`, `readme_check_changed`, `readme_check_stale` pass only because the check fails with the expected text
- [ ] 1.4 Grep inventory in `~/.local/share/bb2-c-work/cli11-callers.md`: exit codes other than 0/77 that CMake or ctest depend on (`EQUAL <n>`, `WILL_FAIL`, `PASS_REGULAR_EXPRESSION`, `expect_rc`, `want.cmake`), callers of the single-dash options of `blitzlib cmdref`, callers that pass free text to `hdftool edit`, other values starting with `-`. Verify: every row names file and line; groups 2-14 use it for C2
- [ ] 1.5 `docs/development.md` ("Host programs") describes the new dispatch (a `main.cpp` example from the fake program, option declaration, help format, exit codes, the README blocks and how to regenerate them from the failing message). Verify: the example in the text is the text of `fake_cli11.cpp` (`diff` of the quoted lines) and its output block equals `fake_cli11 -h`
- [ ] 1.6 Commit "tools: CLI11, cli helper, help and README checks". Verify: C5; no program changed, all results unchanged

## 2. `hunkcheck` (pilot: three small subcommands)

- [ ] 2.1 `tools/hunkcheck/` on CLI11 (`start`, `layout`, `stored`); callers `programs/ab3cli`, `programs/ted`, `programs/ted/tests/oracle`, `tests/ab3`, `tests/corpus/amiga`, `CMakeLists.txt`. Verify: C1-C5; break a hunk layout on purpose in a scratch copy and see the POST_BUILD check fail with status 1; the pilot's lessons (formatter, helper API) are put into design.md and `docs/development.md` before group 3

## 3. `testinfo`

- [ ] 3.1 `tools/testinfo/` (`stats`, `map`, `select`); callers `cmake/coverage-map.cmake`, `cmake/coverage-select.cmake`. Verify: C1-C5; `lcov/*.info` and `selected` of a host coverage run are `cmp`-identical to before; `test_covmap`, `test_covsel`, `test_teststats` unchanged

## 4. `refcheck`

- [ ] 4.1 `tools/refcheck/` (`asm`, `labels` with `-v`/`-q` as excluding flags, `data`, `listing-*`); callers `libs/acidlibs`, `programs/ted`, `programs/blitz2`, `CMakeLists.txt`. Verify: C1-C5; the reports of the four checks `cmp`-identical; a stale intended-list entry still fails

## 5. `results`

- [ ] 5.1 `tools/results/` (`match`, `prev`, `cd-update`, `cd-reference`); callers `tests/ab3`, `tests/bug-orig` (`inject-*.cmake`), `CMakeLists.txt` (`prev_results`), `tools/tests` (`prevcmp_*`, with their message matches), `tests/corpus/CMakeLists.txt`. Verify: C1-C5; `prev_results` passes, 77 without a prev build unchanged; the table of `blitz2-cd-reference` identical

## 6. `blitzres`

- [ ] 6.1 `tools/blitzres/` (`create`, `dump`, `source`); callers `libs/userlibs/rwe/residents*.cmake`, `tools/blitzres/check.cmake`. Verify: C1-C5; the residents the RWE reference build writes are `cmp`-identical

## 7. `blitzlib`

- [ ] 7.1 `tools/blitzlib/` (`join`, `extract`, `numbers`, `check`, `compare`, `cmdref`); `cmdref -group/-name/-last` becomes `--group/--name/--last` in `cmdref-check.cmake`, `cmdref-complete.cmake`, `tools/tests/CMakeLists.txt:136` and every other caller found in task 1.4; callers `cmake/BlitzLibrary.cmake` (used by every library target), `libs/`, `tools/blitzlib`. Verify: C1-C5; **`acidlibs`, `deflibs`, every `.obj` and the site JSON from `cmdref` are byte-identical** (`prev_results`, `cmp`); `rwe_numbers*`, `biglib_check_clash`, `cmdref_not_a_library` (status 1) pass

## 8. `bbsrc`

- [ ] 8.1 `tools/bbsrc/` (`tokens`, `detok`, `comment`, `bb22`, `bb22-info`, `ab3`, `ab3-cmp`); `detok-tooltest.cmake` and `ab3-tooltest.cmake` follow the new messages and pass the status they expect (usage errors 2); callers `libs/tests`, `libs/userlibs/rwe`, `tests/ab3`, `tests/bug-orig`. Verify: C1-C5; token lists and `tests/data/ted-tokens.txt` regenerate byte-identical; `ctest --preset oracle` passes

## 9. `tedsrc`

- [ ] 9.1 `programs/ted/tools/tedsrc/` (`tok`, `detok` with `--check`), linked with `tedhost` and `tedcore_host`; `tests/run.cmake` keeps expecting 1 for a bad source (design D7); callers in `libs/userlibs/rwe`, `tests/`, `tools/bbsrc`. Verify: C1-C5; `teddetok_bblibsrc` and `tedtok_libs` run (not skipped, data from `cmake/fetch-originals.cmake`); tokenised corpus sources `cmp`-identical

## 10. `tedab`

- [ ] 10.1 `tests/ted-ab/runner/` (`run`, `lint`, `info`, `text`, `cmp` or as they are named by then); the verdict codes 0, 1, 3, 4 stay and are in the README and the help footer; callers `tests/ted-ab/CMakeLists.txt`, `tests/bug-orig/CMakeLists.txt` (lines 402-555), `want.cmake`. Verify: C1-C5; `ted_ab` pass (needs Amiberry) gives the same verdicts; `text` dumps `cmp`-identical

## 11. `corpusrun`

- [ ] 11.1 `tests/corpus/runner/` (`pass`, `cd`); `CDRUN_FORCE`, `BUGORIG_RECORD` stay; `check-cdrun-source.cmake` keeps its expected status 2 and follows the new message text; callers in `tests/corpus`, `tests/bug-orig`, `tests/ab3`, `tests/blitz2`, `libs/userlibs/rwe`, `CMakeLists.txt`. Verify: C1-C5; the corpus passes with the same verdicts; `cdrun_source`, `cdrun_cache`, `corpus_runner`, `bugorig_verdict` pass

## 12. `amiga_run` and `mktestdata`

- [ ] 12.1 `tools/amiga_run/` (all options of its header comment: `--machine`, `--rtg`, `--visible`, `--log`, `--timeout`, repeatable `--set/--unset/--mount/--mount-ro`, `--dir`, `--cold`, `SCRIPT|-`, `--purge-snapshots [OWNER]` as the mode that excludes SCRIPT) and `tools/mktestdata/` (`SPEC OUT`); the `amiga_run_purge_*` tests of `tools/tests/CMakeLists.txt:114-129` keep their expectations (status, `WILL_FAIL`). Verify: C1-C5; a script run through `amiga_run` (`ctest --preset emulator -R amiga_emulation`) passes; `mktestdata` output `cmp`-identical on the specs of the tests

## 13. `hdftool`

- [ ] 13.1 `tools/hdftool/` with subcommands `info`, `list`, `cat`, `check`, `edit` on CLI11 (the dispatch of `main.c:21-33` goes), `edit`'s operations in `parse_order()` order (design D5); `host_help_test(hdftool info list cat check edit)` and the README blocks; caller `cmake/setup-amiga-forever.cmake` (values starting with `-`: design Q5). Verify: C1-C5; `tools/lib/hdf` tests unchanged; an image edited with the old and the new `hdftool` on the same operations is `cmp`-identical (test image built in memory by the lib's tests or a fixture)

## 14. `xtra`

- [ ] 14.1 `tools/xtra/` with subcommands `dump`, `check`, `convert` on CLI11 (`--bb22`/`--ab3`/`--source` as options, the exclusive modes with `->excludes()`); `tooltest.cmake` follows the messages; `xtra_tool`, `xtra_originals`, `xtra_ab3_examples` pass. Verify: C1-C5; the `.xtra`/header files the tests write `cmp`-identical

## 15. Final sweep

- [ ] 15.1 Remove `tools/lib/subcmd/`, `tools/tests/fake_subcmd.c`, `test_subcmd.c`, the old `check-help.cmake`/`check-readme.cmake` (the `-cli` scripts take the old names) and the `lib_subcmd` references; `docs/development.md`, `docs/structure.md`, `tests/README.md`, `README.md`, `TODO.md` (a done entry for this change with the commits) name CLI11 and the C++ command layer. Verify: a search for `subcmd`, `subcmd_main` and for `strcmp(argv[` finds nothing outside `archive/`, `TODO.md`, `openspec/` and the test programs that stay (`tools/tests/test_amiga_emulation*.c`); every README block passes `readme_*`
- [ ] 15.2 `docs/development.md` and the READMEs list the exit codes (0, 2, 77, and per program the documented others) and `README.md` (section "License") name CLI11 (BSD-3-Clause) as a build-time dependency of the host tools. Verify: `readme_*` pass; `grep -rn CLI11 README.md` shows the licence line
- [ ] 15.3 Whole-state verification on the last commit versus the commit before group 1: `-DBLITZ2_PREV=<that commit>`, `prev_results`, corpus and `ted_ab` prev parts, `ctest --preset all`, `ctest --preset oracle`, a `-DHOST_SANITIZE=ON` host run, a clean build without warnings. Verify: no difference in any result; failures equal the baseline of 0.1 or fewer
- [ ] 15.4 Archive the change (`openspec archive host-tools-cli11`) after the coordinator accepted the commits. Verify: `openspec/specs/tooling/host-tools/spec.md` has the modified requirements

## Workflow follow-up

- GitHub: no issue is closed by this change; the coordinator files the
  CLI11 dependency note and the label handling if wanted.
