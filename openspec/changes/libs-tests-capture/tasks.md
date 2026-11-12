# Tasks

Groups 1-2 build the new mechanism beside the tape; groups 3-8 convert the
programs (a tape is deleted in the commit that converts its program, design
D10); group 9 removes the tape; group 10 documents. After every task
`ctest --preset all` stays green (the CD-less run). No library or compiler
output changes (`prev_results`).

## 1. Measure and decide

- [ ] 1.1 Count the checks per program: run `libs_emulator_originals` (needs the CD) with the program's own summary line (`NAME: N checks, M failures`) and write the 62 numbers to the task's commit message or a scratch file for D10. Verify: every program has a number
- [ ] 1.2 Re-run the address experiment of task 9.8 (a padded `libs_tests` binary and a heap shift) with a throwaway patch to list the observations that hold addresses; the list names the sites for `base+$OFF` or `prog`. Verify: the list is in the commit message of 2.2 and every site is converted in its group
- [ ] 1.3 Measure the text volume: write the observations of one mid-size program (`systest_mathstr`, `libtest ldivlib`) as lines with a throwaway patch and extrapolate the total; decide block vs lines for `libtest` and `systest_med` and record the decision in design.md Open Questions. Verify: design.md no longer lists the two questions as open

## 2. Infrastructure beside the tape

- [ ] 2.1 `libs/tests/capture.[ch]`: `cap_begin/cap_end`, line writer with a static buffer, number/bytes/string/pointer formatters, `not captured`, block lines, `LIBS_CAP_DUMP`. Verify: a host-compiled or emulator unit program (`libs_capture_format`) writes known lines and the expected file text is compared
- [ ] 2.2 `LIBSET` in `libs_tests`/`sysload.h`: `load_set` loads one file set; unconverted programs keep `load_sets`. Verify: `ctest --preset all` unchanged
- [ ] 2.3 `capture-compare.cmake` with the exceptions table (`libs/tests/exceptions.tsv`, header as `tests/corpus/notes.tsv`) and the `SAME` mode. Verify: the tests of D8 on fake files fail and pass as stated (`libs_capture_compare_same`, `_value`, `_missing`, `_extra`, `_block`, `_row_*`, `_not_captured`)
- [ ] 2.4 `emulator.cmake`: `LIBSET`, the `captures/` output directory, the compare step after the run, `-DSAME=ON` for the originals run; wire into `libs_emulator` (`obj`) and `libs_emulator_originals` (`orig`). Verify: with one program converted (2.5) both tests pass and name its compare in their log
- [ ] 2.5 Convert `libtest.c` (ldivlib, lmullib) as the first program, with the block/lines decision of 1.3; delete `libtest_*.tape`; commit its capture from the originals run. Verify: `libs_emulator` passes without the CD; `libs_emulator_wrong` (changed line) fails with the line's message; two capture runs are byte-identical (`libs_capture_repeat`)

## 3. Framework macros

- [ ] 3.1 `sysload.h`: `OBS`, `OBSBYTES`, `OBSSTR`, one `struct set`, required-kept check with `kept`/`lost` lines and the refusal of `lost` in a capture run (D6), `check()` stays for the program's own consistency checks. Verify: host/CMake test `libs_capture_lost` (a `lost` line in a capture is refused) and the emulator test of a routine that loses a register (`libs_emulator_lost`, a build with a broken thunk from a variant file)
- [ ] 3.2 `rwe/rwecmp.h`: the same for `DO2`, `DO2S`, `preserve`, `KEEPS`, `rwe_done` (the summary table of kept registers stays as the aid). Verify: the first converted rwe program (4.1) compiles and passes

## 4. RWE userlibs A (alphabetical)

- [ ] 4.1 `bb_blitlib`, `blitzbomberslibtwo`, `ciatrklib` (with the determinism decision of D7 for the F00 phase), `neilsreqtoolslib`, `popupwinlib`. Verify: per program the number of checks equals task 1.1's, the capture is committed, `libs_emulator` passes, two originals runs give the same file
- [ ] 4.2 `riamosfunclib`, `rianimlib`, `riapplib`, `riarexxlib`, `riarrayslib`, `ribbmaplib`, `richunkylib`. Verify: as 4.1
- [ ] 4.3 `ricommoditieslib`, `ricompactdisclib`, `ricopperfxlib`, `ridatatypeslib`, `ridebuglib`, `ridisplaylib`, `ridoslib`. Verify: as 4.1
- [ ] 4.4 `ridisasmlib` with block lines (`cap_blocks`), its code buffer at the fixed address stays. Verify: as 4.1; a changed block hash fails naming the block (D8 `_block`); the capture file is smaller than 1 MB

## 5. RWE userlibs B

- [ ] 5.1 `riencryptlib` with the seeding or `not captured` decision of D7. Verify: as 4.1; two capture runs byte-identical
- [ ] 5.2 `rievallib`, `rifastseriallib`, `rifnslib`, `rifxlib`, `rigfxlib`, `rigtmenulib`, `riintuilib`. Verify: as 4.1
- [ ] 5.3 `rilesdebuglib`, `rilinkedlistlib`, `rilvcallhooklib`, `rimapeditorlib`, `rimapshapeslib`, `rimisclib`, `rinonvolatilelib`. Verify: as 4.1
- [ ] 5.4 `ripacklib`, `riregistrylib`, `rireqlib`, `riseriallib` (its patched-address counts become observations of the one set, not `tape_orig()`), `rishapeslib`, `rishapezlib`, `risnowlib`. Verify: as 4.1
- [ ] 5.5 `risortlib`, `risoundlib2`, `risoundlib`, `ristcdecrunchlib`, `ristonecrackerlib`, `rithreadlib`, `ritooltypeslib`, `ritrackdisklib`, `ritrackerlib`, `riwbspritelib`, `rizonejoylib`. Verify: as 4.1

## 6. systest programs

- [ ] 6.1 `systest.c` and `systest_runtime.c`. Verify: as 4.1
- [ ] 6.2 `systest_mathstr.c` and `systest_system.c`. Verify: as 4.1
- [ ] 6.3 `systest_cmdlibs.c` (the sort order comparison becomes an observation of the order). Verify: as 4.1
- [ ] 6.4 `systest_med.c` with block lines (3.57 million observations). Verify: as 4.4

## 7. Exceptions and the helper's tests

- [ ] 7.1 Remove `selftest_fixed.c`, `expect_fixed`, `check_fixed`, `SAME_FIXED`, `fixed_verdict`; `libs_expect_fixed*` become the `libs_capture_row_*` tests of 2.3 (already present). Verify: `ctest --preset all -N | grep expect_fixed` is empty; the row tests pass
- [ ] 7.2 `libs_emulator_wrong` (changed line in an emulator run) and `libs_emulator_repeat` as in D8. Verify: the first fails with the line message, the second passes

## 8. Tapes out

- [ ] 8.1 Delete `tape.c`, `tape.h`, `tape-compare.cmake`, `tapes/`, `LIBSTAPE`/`LIBSTAPEDUMP`, the replay copy in `emulator.cmake`, `tape_*` calls and the `unconverted` branch of `load_sets`; delete `libs_emulator_tape_wrong`, `libs_tapes_compare_*`. Verify: `grep -rn "tape" libs/tests libs/acidlibs CLAUDE.md tests/README.md` finds no current mention (the archived change is history); `ctest --preset all` passes; a clean rebuild has no warnings
- [ ] 8.2 Acceptance: `ctest --preset all` and, with the CD, `ctest --preset originals` and `ctest --preset bug-orig`, plus `prev_results` (library files byte-identical). Verify: the three runs pass as before the change; the number of checks per program equals task 1.1's

## 9. Documents

- [ ] 9.1 `tests/README.md` "Libraries": single-library runs, capture format, `captures/`, renewing a capture, the exceptions table, the kept-register rule, `not captured`, the rule "a fix does not change the scenario". Verify: the documented commands run as written
- [ ] 9.2 `CLAUDE.md` (names the tapes), `libs/acidlibs/brexxlib.c` and the test programs' comments that name the tape, the presets table. Verify: `grep -rn "tape" CLAUDE.md libs tests/README.md docs` shows nothing current
- [ ] 9.3 Sync the delta to the main specs (`openspec-sync-specs`) and archive after the acceptance. Verify: `openspec validate --specs` passes

## Out of scope

- Setup/act/verify asserts for the pure compute libraries: a later separate
  change, which can replace single captures by stated expectations.
