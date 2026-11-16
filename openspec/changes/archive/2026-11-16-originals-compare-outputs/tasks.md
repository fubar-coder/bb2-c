## 1. Specs

- [x] 1.1 Write the delta specs: `testing/original-comparisons` (new requirement), `testing/expected-results` (the CD table rows, the corpus pair, record and update, the broken-input list), `testing/bug-fix-workflow` (what a fix commit names). `testing/test-model` states no executable rule and stays. Verify: `openspec validate originals-compare-outputs` passes.

## 2. Corpus

- [x] 2.1 Remove the observation `exe` from `tools/lib/corpusexpect` (names, `cexp_update_exe`, the record, the projection) and from `corpusrun pass` (`--observe` writes no `NAME.exe`, `--update-expect port` is an ordinary update). A `NAME.exe`, `NAME.orig.exe`, `NAME.fixed-N.exe` is an unknown observation. Verify: `test_corpusexpect` (broken input: an `.exe` file alone, as a pair, in a run).
- [x] 2.2 Add the configuration expectation `exeinfo` (`port` of `*_originals`): the output is judged, the executable is compared with ref's for information, none at all fails. Verify: `test_corpus` (parse), `corpus_originals_exe_differs` (passes, reports) and `corpus_originals_exe_differs_output` (a wrong output fails).
- [x] 2.3 Remove `t130_blit.orig.exe` and `t130_blit.fixed-46.exe` of `corpus` and `corpus_runerr` (four files); `update.cmake` and `record.cmake` write no executables. Verify: `corpus_record_target`, `corpus_update_fix`, `corpus_expect_malformed_exe`, `corpus_expect_malformed_exe_pair`.

## 3. CD table

- [x] 3.1 `corpusrun cd` compares result, line and message with the reference table (`tests/corpus/runner/cdcmp.c`); size and hash of an ok row are reported and counted. Verify: `cdrun_compare` (different result, error line, message, missing row fail; another executable alone passes).
- [x] 3.2 `results cd-update` keeps the old size and hash of a row with the same compile result, counts the rows with another executable, and drops a hash note for a source the port still compiles. Verify: `cdtable` (host test).
- [x] 3.3 The two obsolete hash notes of the table (`#969`, `#1048`, the port compiles both) are dropped and the rows hold the original's size and hash again (`blitz2-cd-update`, then `blitz2-cd-reference`). Verify: `blitz2_cd_reference`, `corpus_cd`.

## 4. TED A/B

- [x] 4.1 Pass key `exe_info=1` of `ted_ab_originals`: a differing executable of `Cc` and `Rc` (`@expect corpus`) and of a file in the pairs `R5-Rc`, `C5-Cc` is informational. Verify: the cases `exe_info` of `ted_ab_tedab` (strict without the key, strict for `R5`, `C5`, other files).

## 5. Documents

- [x] 5.1 `tests/README.md`, `docs/testing.md`, `docs/development.md`, the runner and `results` READMEs. Verify: `host_help_test` and `host_readme_test` of `corpusrun` and `results`.
- [x] 5.2 Run `ctest --preset host`, `ctest --preset originals`, `ctest --preset all`: all 746 pass (745 before: +`corpus_expect_malformed_exe_pair`); originals the same set except `corpus_originals_exe_unlisted` (now `corpus_originals_exe_differs` and `corpus_originals_exe_differs_output`), the corpus passes `corpus_originals` and `corpus_runerr_originals` pass (the second failed on `t130_blit`'s executable pair before); three `bug_orig_record_compiler_*` macro tests fail on main as before.
