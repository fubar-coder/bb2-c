# Tasks

One topic per commit (CLAUDE.md, "Git"): each numbered task that changes
files is one commit unless it says otherwise. No commit of this change
changes a result of rule 3: every one is checked with the prev build and
without `PREV_INTENDED` (design.md D12). Agents run the tests their task
names, the tests of what they touched and `ctest --preset host`; no full
preset runs except the measurement of 8.3 (D-026). The user's answers:
rounds/tree decisions, "2026-11-27 ab3-test-model Q1-Q5" (design.md D8, D10, D14, D15).

## 1. testpp: `@@` and the AB3 header

- [x] 1.1 `tools/lib/testpp`: the source mode of a test source (AB3 or BB2, given by the caller); in AB3 mode a line ending in `@@` outside strings and comments joins the next for the parsing of the macros (the rule of `ab3syn.c` `findjoin`/`ext_nextline`); a check macro so continued is written on one line with empty lines after it; other lines unchanged. Verify: host tests of testpp (a check continued with `@@` once and twice, `@@` inside a string and after `;`, `@@` outside a check kept, the source line of the check in the table, an unclosed macro at the last line refused naming file and line), also with `-DHOST_SANITIZE=ON`.
- [x] 1.2 testpp: in AB3 mode the leading `; KEY = VALUE` block of the source as the first lines of the driver. Verify: host test (a header of three lines first in the driver; a source without header unchanged; a header after code is no header).
- [x] 1.3 Byte check of the existing drivers: the drivers of `model-drivers/` of `tests/blitz2` and `tests/bug-orig` written before and after 1.1/1.2 are identical (a host test with a BB2 source holding `@@` that must stay as is, and a diff of the build's `model-drivers/` before/after, named in the commit). Verify: the diff is empty; the prev comparisons of the model (`ctest -R '_prev$' -L emulator` on the model tests) pass without `PREV_INTENDED`.

## 2. The run configuration

- [x] 2.1 `tools/lib/testconf`: `compiler` values `ab3`, `amiblitz3`; run setting `source-mode` (`ab3`, `bb2`; refused with another compiler, `bb2` refused with `amiblitz3`); in `deviations."SELECTOR"` the key `variant` (required where the selector names only `amiblitz3` cells, refused elsewhere and in a `tests.toml`); `!ExpectError` with an `amiblitz3` cell refused. `--print-config` shows them. Verify: `test_testconf` with broken input for every refusal naming file and line; `testconf_readme`.
- [x] 2.2 `tools/lib/testreg` and `cmake/basic-test-model.cmake`: the side `oracle` (`PREFIX_ID__CASE_oracle`, label `oracle`, never `emulator`, never `bug-orig`), the directory `tests/ab3/model/` with prefix `ab3_`, its drivers not in `BASIC_TEST_DRIVERS` of `ab3_bb2_equivalence`. Verify: `testreg_tool` host tests (a test with both cells gives two tests per case with the right labels; a deviation with `variant` gives no `bug-orig`); `testreg_readme`.
- [x] 2.3 The build check of `variant` values against the first column of the rows of `docs/ab3/variant.md` (in the registration step; a test with a broken value fails the build naming test and value). Verify: the broken-input test.

## 3. The runner

- [x] 3.1 `tests/corpus/runner/caserun.c` and the session script: the staging of the default `Blitz3:` tree (as `cli.cmake`'s tree `Blitz3`, testlib in `UserLibs/`) as one function the runner's other passes can call; the command-line compile of the ASCII driver (`ID.ab3`/`ID.bb`) for `ab3` and `amiblitz3` with `-r` from `runtime-errors`; the re-assign of `Blitz3:` before every AmiBlitz3 compile; no JIT in sessions with an `amiblitz3` cell; skipped (77) without the fetched files. Verify: `test_caserun.c` host tests of the session script for both compilers; an emulator run of the spike's `one` case in the new form on both compilers (the result file of the spike report, section (a)).
- [ ] 3.2 The `compile` and `compile.log` queries of a command-line compile (D4): the executable's existence, blitz2-ab3's `FILE:LINE: MESSAGE` mapped to the test source, AmiBlitz3's first error line. Verify: host tests of the extraction (both forms, no executable with rc 0, a warning that is no error); an emulator test with a deliberately broken driver line on both compilers.
- [ ] 3.3 Bundles (D14): the session key of `compiler = "ab3"` with default settings may form bundles, never mixed with `port` keys; the `amiblitz3` cells never form bundles (Q4). Verify: `bundle-selftest` extended with two `ab3` sources in one bundle, a `port` source in another and an `amiblitz3` cell in none; host tests of the key.
- [ ] 3.4 Time limits: `timeout` and `hang-after` of `tests/ab3/model/tests.toml` from a measurement of five `amiblitz3` sessions (a silent compile under load). Verify: the measured values in the commit message.

## 4. Prev comparison

- [ ] 4.1 `cmake/basic-test-model.cmake`/prevcmp: the tests of `tests/ab3/model/` compared with the prev build (`ab3_ID_prev`, blitz2-ab3 of both commits, once per `runtime-errors` value of the `ab3` cells; `amiblitz3` not compared); entries `ab3_ID_prev:ID[-runerrs]` checked as stale like the others. Verify: `cmake --preset amiga -DBLITZ2_PREV=ON`, the comparison of the pilot test passes; with an entry for an unchanged test it fails as stale.

## 5. Pilot

- [ ] 5.1 The conversion table for all cases of D10 (Q1): per case its target test file, the form (checks or `output`), each `[run]` line with its check name and expected literal, the AmiBlitz3 side (`REF` or excluded, `.ref.expect` lines as deviations with their `variant` row); written to `~/.local/share/bb2-c-work/ab3-test-model/conversion.md` and reviewed by the coordinator before group 6. Verify: every case of D10's "move" list has a row; no case of the "stays" list has one.
- [ ] 5.2 Pilot commit: `tests/ab3/model/tests.toml` and five cases moved (one plain check case, one with `@@` in its own statement (chr_at), one `output` case (esc_basic), one with an AmiBlitz3 deviation (quick_long_arith), one with AmiBlitz3's "no executable" (chr_nospace)); their `ab3_cli_case` lines, sources and expect files removed. Verify: D12 point 3 (a)-(d) for the five; `ctest -R '^ab3_'` on the new tests and `-R '^ab3_cli_[0-9]'`; `ctest --preset oracle -R '^ab3_'`; the prev comparison of 4.1.

## 6. The moves by topic

Each task one commit: the cases of the topic moved with the proof of D12
point 3, their old cases removed. Verify for each: the proof (a)-(d) for
every case of the commit (the one-off comparison's output in the report),
the new tests on both sides, the remaining `ab3_cli_N`/`ab3_cli_ref_N`,
the prev comparison of the new tests.

- [ ] 6.1 `proc_*` and `funcstmt_*` (by `OPTIMIZE`/`SYNTAX` groups; `proc_fastname` and others whose names are the subject in files of their own).
- [ ] 6.2 `macthen_*`, `macend_*` (but `macend_inc`), `macro_ns`, `label_*`, `underscore`, `dotlabel`, `shared_global`, `const_novalue`.
- [ ] 6.3 `esc_*` (but the include cases of D10), `str_expr`, `tilde_string`, `latin1_string`, `chr_*`.
- [ ] 6.4 `byte_*`, `quick_long_arith`, `intrinsic_*`, `pow`, `pow_negative_base`, `expr_depth`, `opt_*`, `libtoken_past_block`, `asm_short_s`.
- [ ] 6.5 The header cases (`runerrs`, `debugconst_*`, `cnifdef`, `ptr_accept`, `esc_ver`) and the `-r` cases; `join`, `join_op`, `join_funcpar`, `comment_code`, `plus`, `tokens`.
- [ ] 6.6 The BB2-mode cases with `source-mode = "bb2"` (`inctext_bb2` stays by D10; `ptr_bb2`, `esc_bb2`, `esc_dcb_bb2`, `macend_bb2`, `proc_bb2_fast`, `asm_short_s_bb2`).
- [ ] 6.7 After the last move: `tests/ab3/CMakeLists.txt` and `cli/` hold only the cases of D10's "stays" list and the other kinds; the comments of `ab3_cli_case` and `cli.cmake` say so. Verify: the one-off count of design.md Context gives only those kinds; `ab3_cli_broken*` and `ab3_cli_prev_*` pass.

## 7. The expression matrix (Q2)

- [ ] 7.1 `tools/lib/exprmatrix`: `exprmatrix expect` writes the AB3 cases as tests of `tests/ab3/model/exprmatrix/` (one case per generated source, checks named by the cell's key, the `Print` cells by `output`, AmiBlitz3's values as deviations with their `variant` row, `runtime-errors = "off"` for `-r`) instead of `cli/exprmatrix/*.ab3`, `.expect`, `.ref.expect`, `cases.cmake`. Verify: its host tests; the generator's check of the generated files; D12 point 3 for every source by the one-off comparison.
- [ ] 7.2 The old `cli/exprmatrix/` removed with the include in `tests/ab3/CMakeLists.txt`. Verify: the new tests on both sides; the time of the expression matrix's AB3 part before and after (spec "The matrix's tests are not slower than the old form", targeted `-R` runs on both commits).

## 8. Docs and measurement

- [ ] 8.1 `tests/README.md`: "The test model" (tests/ab3's run-output cases moved, what stays and why), "The run configuration" (`compiler` values, `source-mode`, `variant`), "Original material and presets" (`oracle` holds both forms), "The prev build" (`ab3_ID_prev`), "bug-port tests" (D16), the guide for an AB3 test (grouping, `SHARED`, `FAST`, `@@`, the header); `docs/testing.md` where it describes tests/ab3; `docs/ab3/variant.md`: the case names in the rows point to the model tests; `tools/testreg/README.md`, `tools/lib/testconf` README if any. Verify: `testconf_readme`, `testreg_readme`, the docs checks of the build.
- [ ] 8.2 The Purpose of `openspec/specs/testing/test-model/spec.md`: the "Not yet moved" item of `tests/ab3` names only what stays (sync with the archive; a task note for the archive if the sync does it). Verify: `openspec validate ab3-test-model --strict`.
- [ ] 8.3 Measurement: the time of the tests/ab3 share of `all` (the `ab3_cli_N` blocks before; the new port tests, their bundles and the remaining blocks after) and of `oracle`, by targeted `-R` runs on the commit before group 5 and after group 6/7, same machine. Verify: the numbers in the report; slower than before: say so to the coordinator (the user's answer of 2026-11-23 on the speed of tests of the new method).
- [ ] 8.4 Clean build without warnings; `ctest --preset host`; the tests of `tests/ab3` (`-R '^ab3_'`) and `ctest --preset oracle`. Verify: the report lists them; the full acceptance is the coordinator's (D-026).

## 9. Other changes (notes for the coordinator; no edit of their files here)

- [ ] 9.1 `ab3-mode-g5` before it starts (recorded order): only g5's new run-output cases take the new form (Q5 of rounds/tree "2026-11-27 ab3-test-model Q1-Q5", "Nur Ausgabefälle neu (Recommended)"); its tasks 2.1, 2.2, 2.3, 2.4, 3.1, 3.3, 5.2, 5.3, 5.5 add run-output cases "in `ab3_cli_ref`/`ab3_cli`": they are written as cases of `tests/ab3/model/` (both cells; AmiBlitz3's value as a deviation with its `variant` row); 2.5, 4.3 (console line, rc 5, `-werr`), 5.1 (an include relative to the main source and the last path) and a compile-error probe stay `ab3_cli`/`ab3_cli_ref`; 1.2 calls the runner's `Blitz3:` staging of 3.1 instead of a copy; 1.4 "the existing `ab3_cli_ref` setup" reads "the `amiblitz3` cells or `ab3_cli_ref`"; 2.6 "the existing `ab3_cli`, `ab3_cli_ref` cases are unchanged" also names the tests of `tests/ab3/model/`; 6.2's `oracle` run covers both forms. Verify: the coordinator (or the planning agent with `openspec-update-change` on g5) applies the wording; `openspec validate ab3-mode-g5 --strict`.
- [ ] 9.2 `blitz-coverage` task 3.2: the coverage switch also for the `ab3` cells of `corpusrun case`. `console-break-option` task 2.2: its cases have arguments other than `-r` and stay `ab3_cli`; only the case list of `tests/ab3/CMakeLists.txt` is shared. Verify: a note in the report to the coordinator.

## Workflow follow-up

- Archive with `openspec-archive-change`: creates `testing/ab3-tests`,
  changes `testing/basic-test-runner`, `testing/previous-commit` and
  `testing/expression-matrix`; the Purpose of `testing/test-model`
  as in 8.2.
