# Tasks

Groups 1-2 build the new mechanism beside the tape; groups 3-8 convert the
programs (a tape is deleted in the commit that converts its program, design
D11); group 8 removes the tape; group 9 rewrites the documents.
After every task `ctest --preset all` stays green (the CD-less run). No
library or compiler output changes (`prev_results`). Names: the tests
`libs_emulator` and `libs_emulator_originals` keep their names (they only
compare); new targets `libs-capture` (record from the original) and
`libs-update` (update from the port); `libs_emulator_wrong` (was
`libs_emulator_tape_wrong`); the judge is `tools/results libs-compare`,
`libs-accept`, `libs-update`. Terms as in `testing/test-model`: record =
from the original, update = from the port, "expected data", never "the
reference".

## 1. Measure and decide

- [x] 1.1 Count the checks per program: run the originals test (needs the CD) with the program's own summary line (`NAME: N checks, M failures`) and write the 60 numbers to the task's commit message or a scratch file for D11. Verify: every program has a number Done (2026-11-12): measured; the facts are in design.md, "Measurements (group 1)".
- [x] 1.2 Re-run the address experiment of task 9.8 (a padded `libs_tests` binary and a heap shift) with a throwaway patch to list the observations that hold addresses; the list names the sites for `base+$OFF` or `prog`. Verify: the list is in the commit message of 2.2 and every site is converted in its group Done (2026-11-12): measured; the facts are in design.md, "Measurements (group 1)".
- [x] 1.3 Measure the committed text size: write the observations of one mid-size program (`systest_mathstr`, `libtest ldivlib`) as cases with a throwaway patch, extrapolate the total (blocks for `systest_med` and `rwe_ridisasmlib` only, plain cases for the rest) and measure it compressed too (`git`'s pack size, e.g. `gzip -9` as an estimate); check whether the strings and hex buffers written hold literal CD data (tables, texts) by reading a sample of the longest values of the programs that read the CD's libraries. Decide: keep text, or hash more programs; record the decision and the measured sizes in `design.md` (Risks), naming that it reverses the argument of task 9.8 of the archived change. Verify: the numbers and the decision are in the commit message and `design.md`; the user is told if the size or the CD content is unacceptable Done (2026-11-12): measured; the facts are in design.md, "Measurements (group 1)"; the decision (keep text or hash more) is left to the user, Open Questions.
- [x] 1.4 List today's non-deterministic checks: run every program several times (at least 5) against the original, with the tape recording mode and with a different `LIBS_PAD`-like heap shift (throwaway patch), and list the checks whose observation differs between runs (the tape segment hashes or the summary lines show it). Verify: the list (program, check, reason) is in the commit message of 2.2; each entry has a planned treatment (fixed seed, defined state, `not captured: REASON`) in the group of its program Done (2026-11-12): measured; the facts are in design.md, "Measurements (group 1)".

## 2. Infrastructure beside the tape

- [x] 2.1 `libs/tests/capture.[ch]`: `cap_begin/cap_end`, the case writer with a static buffer, the key (routine, check, vector, occurrence number; escaping of TAB, `|`, `\` and non-printables), number/bytes (hex up to 64 bytes, longer as length + FNV)/string/pointer formatters, `not captured`, block cases, `LIBS_CAP_DUMP` (named key or `BLOCK:N`). Verify: a host-compiled or emulator unit program (`libs_capture_format`) writes known cases (a 64-byte and a 65-byte buffer, a block, the same check twice, a key with `|` and TAB) and the expected file text is compared
- [ ] 2.2 `LIBSET` and `LIBS_PAD` in `libs_tests`/`sysload.h`: `load_set` loads one file set; the padding allocation shifts the layout; unconverted programs keep `load_sets`. Verify: `ctest --preset all` unchanged
- [x] 2.3 The judge as a C tool: `tools/lib/libscmp` and `tools/results libs-compare` (qualified cases per `--set`, keys missing and added, block, long buffer, `not captured`, malformed/duplicate/ambiguous files with the line, `--only`), with `tools/tests/test_libscmp.c` from memory for every case of D9, and `libs_compare_wrong_*` ctest runs on fake files. Verify: each broken input fails with the check's message and the good input passes; the judge compares two 3-million-case files in one pass in a few seconds
- [ ] 2.4 `libs-accept` and `libs-update` in the same tool and the targets `libs-capture` (two runs with `LIBSET=orig` and different `LIBS_PAD`, accept only if identical, else "test not deterministic" with the differing cases and nothing written; refuse a `lost` case; keep `[fixed #N]` cases; filter `-DFILTER`) and `libs-update` (one run with `LIBSET=obj`, rewrite `[fixed #N]` values, `-DFIX=KEY#N` makes the pair; no CD). Verify: host tests of accept equal/different/`lost`, of update with and without `--fix` and of the filter pass; `libs_accept_nondeterministic` leaves the committed directory unchanged
- [ ] 2.5 `emulator.cmake`: `LIBSET`, the `captures/` output directory kept in the work directory, the `results libs-compare` step after the run; wire into `libs_emulator` (`--set obj`) and `libs_emulator_originals` (`--set orig`); add the rule `libs/tests/expected/**` in `tools/testinfo/rules.txt` (both tests) and `/libs/tests/expected/** -text diff` in `.gitattributes`. Verify: with one program converted (2.6) both tests pass and name the compare in their log; `testinfo` selects both tests for a changed expected file
- [ ] 2.6 Convert `libtest.c` (ldivlib, lmullib) as the first program, block cases (D6); delete `libtest_*.tape`; commit its expected data from `libs-capture` (original). Verify: the C test passes without the CD; `libs_emulator_wrong` (changed case) fails with the key's message; a repeated `libs-capture` gives the same file
- [x] 2.7 The fix-scope check: `tools/results fix-scope` (logic in the judge library of 2.3) gets the expected data of `main` and of the branch and the issue numbers of the branch's `Fixes #N` lines, and reports every added, changed or removed case that is not a `[fixed #N]` case of a named issue or its paired `[orig]` case; a CMake target `fix-scope` (`-DBASE=main`, default `main`) exports the base files with git and runs the tool (orchestration only). Verify: host tests from memory: only own cases pass; a changed unqualified case, a changed `[fixed #M]` case and a change without any `Fixes` line are each reported with the key; broken input (malformed file) fails

## 3. Framework macros

- [ ] 3.1 `sysload.h`: `OBS`, `OBSBYTES`, `OBSSTR`, one `struct set`, the stated-kept check with `kept`/`lost` cases (`KEEPS(routine, regs)`, D7), `check()` stays for the program's own consistency checks (a `check()` that a fix touches becomes an `OBS`). Verify: the accept test of `lost` (2.4) refuses a `lost` result, and the emulator test `libs_emulator_lost` (a routine that loses a stated register, from a variant file with a broken thunk, gives `lost` and fails)
- [ ] 3.2 `rwe/rwecmp.h`: the same for `DO2`, `DO2S`, `preserve`, `KEEPS`, `rwe_done` (the summary table of kept registers stays as the aid for the stated sets). Verify: the first converted rwe program (4.1) compiles and passes

## 4. RWE userlibs A (alphabetical)

- [ ] 4.1 `bb_blitlib`, `blitzbomberslibtwo`, `ciatrklib` (with the determinism decision of D8 for the F00 phase), `neilsreqtoolslib`, `popupwinlib`. Verify: per program the number of checks equals task 1.1's, the expected data is committed by `libs-capture` (which accepted it, so two layouts agree), `libs_emulator` passes, a stated kept set is the intersection of the original's calls
- [ ] 4.2 `riamosfunclib`, `rianimlib`, `riapplib`, `riarexxlib`, `riarrayslib`, `ribbmaplib`, `richunkylib`. Verify: as 4.1
- [ ] 4.3 `ricommoditieslib`, `ricompactdisclib`, `ricopperfxlib`, `ridatatypeslib`, `ridebuglib`, `ridisplaylib`, `ridoslib`. Verify: as 4.1
- [ ] 4.4 `ridisasmlib` with block cases (`cap_blocks`), its code buffer at the fixed address stays; the opcode words stay data of the cases. Verify: as 4.1; a changed block hash fails naming the block (host test of 2.3); the expected file is smaller than 1 MB

## 5. RWE userlibs B

- [ ] 5.1 `riencryptlib` with the seeding or `not captured` decision of D8. Verify: as 4.1
- [ ] 5.2 `rievallib`, `rifastseriallib`, `rifnslib`, `rifxlib`, `rigfxlib`, `rigtmenulib`, `riintuilib`. Verify: as 4.1
- [ ] 5.3 `rilesdebuglib`, `rilinkedlistlib`, `rilvcallhooklib`, `rimapeditorlib`, `rimapshapeslib`, `rimisclib`, `rinonvolatilelib`. Verify: as 4.1
- [ ] 5.4 `ripacklib`, `riregistrylib`, `rireqlib`, `riseriallib` (its patched-address counts become observations of the one set, not `tape_orig()`), `rishapeslib`, `rishapezlib`, `risnowlib`. Verify: as 4.1
- [ ] 5.5 `risortlib`, `risoundlib2`, `risoundlib`, `ristcdecrunchlib`, `ristonecrackerlib`, `rithreadlib`, `ritooltypeslib`, `ritrackdisklib`, `ritrackerlib`, `riwbspritelib`, `rizonejoylib`. Verify: as 4.1

## 6. systest programs

- [ ] 6.1 `systest.c` and `systest_runtime.c`. Verify: as 4.1
- [ ] 6.2 `systest_mathstr.c` and `systest_system.c`. Verify: as 4.1
- [ ] 6.3 `systest_cmdlibs.c` (the sort order comparison becomes an observation of the order). Verify: as 4.1
- [ ] 6.4 `systest_med.c` with block cases (3.57 million observations). Verify: as 4.4

## 7. Fixed checks and the helper's tests

- [ ] 7.1 Remove `selftest_fixed.c`, `expect_fixed`, `check_fixed`, `SAME_FIXED`, `fixed_verdict`; `libs_expect_fixed*` are replaced by the pair-of-cases tests of 2.3 (qualifier rules). Verify: `ctest --preset all -N | grep expect_fixed` is empty; the qualifier tests pass
- [ ] 7.2 `libs_emulator_wrong` (changed case in an emulator run, one program via `--only`) is in place from 2.6; add a `[fixed #N]` demonstration to the host tests (a regressed port fails, a changed original fails). Verify: both fail with the key message

## 8. Tapes out

- [ ] 8.1 Delete `tape.c`, `tape.h`, `tape-compare.cmake`, `tapes/`, `LIBSTAPE`/`LIBSTAPEDUMP`, the replay copy in `emulator.cmake`, `tape_*` calls and the `unconverted` branch of `load_sets`; delete `libs_emulator_tape_wrong`, `libs_tapes_compare_*`. Verify: `grep -rnE "tape\.h|tape_|LIBSTAPE|tapes/" libs/tests libs/userlibs CLAUDE.md tests/README.md docs TODO.md cmake` finds no current mention of the test tape (the archived change is history; the BREXX tapes of `libs/acidlibs/brexxlib.c`, `windowslib.c` and the graphics libraries are not the test tape); `ctest --preset all` passes; a clean rebuild has no warnings
- [ ] 8.2 Acceptance: `ctest --preset all` and, with the CD, `ctest --preset originals` and `ctest --preset bug-orig`, plus `prev_results` (library files byte-identical). Verify: the three runs pass as before the change; the number of checks per program equals task 1.1's

## 9. Documents

- [ ] 9.1 `tests/README.md` "Libraries" and the presets table: one paragraph on the shared test model for all suites (cases, pair `[orig]`/`[fixed #N]`, record against update, "expected data" and "recorded original", `ref` = the original program), then single-library runs, the case format and key, `libs/tests/expected/`, the operations (record with `libs-capture` and reading its "test not deterministic" report, update with `libs-update`, the tests that only compare, the port's output in the work directory for diffing), the stated kept sets and their accepted limit, `not captured`, the limits (block programs, fixes that shift occurrence numbers, `check()` touched by a fix, determinism over memory layout only), the rule "a fix does not change the scenario". Verify: the documented commands run as written
- [ ] 9.2 `CLAUDE.md` (the fix steps under "Original bugs are fixed deliberately" name the fix-scope check: run by the agent, named in its report, run again by the coordinator before the merge), `TODO.md`, `docs/libs/`, `docs/rwelibs/README.md`, `docs/v2.10/libs.md`, `libs/userlibs/rwe/CMakeLists.txt` and the test programs' comments that name the test tape (the grep of 8.1 decides). Verify: the grep shows nothing current
- [ ] 9.3 `testing/original-comparisons` (optional one-line MODIFIED naming `libs_emulator_originals` among the tests of the label `originals`), if the sync finds the list worth completing. Verify: `openspec validate libs-tests-capture` passes
- [ ] 9.4 Sync the deltas to the main specs (`openspec-sync-specs`) and archive after the acceptance. Verify: `openspec validate --specs` passes

## Follow-ups (separate changes, not in this one)

- bug-orig to the shared model: `bug_orig_record_ID` becomes a compare,
  a recording target accepts only agreeing runs (optionally with the
  `pad` of `tests/bug-orig/amiga/inject.c` for the second run), known
  varying files get a wildcard with a reason.
- Corpus to the shared model: `notes.tsv` rows become `[orig]` /
  `[fixed #N]` cases.

## Out of scope

- Setup/act/verify asserts for the pure compute libraries: a later
  separate change, which can replace single recordings by stated
  expectations.
- Libraries with no original (port-only) and checks beyond what the
  original does: the same later change.
