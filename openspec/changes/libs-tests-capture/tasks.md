# Tasks

Groups 1-2 build the new mechanism beside the tape; groups 3-8 convert the
programs (a tape is deleted in the commit that converts its program, design
D10); group 8 removes the tape; group 9 renames the tests and documents.
After every task `ctest --preset all` stays green (the CD-less run). No
library or compiler output changes (`prev_results`). Names after the
change: `libs_test` (was `libs_emulator`), `libs_test_originals` (was
`libs_emulator_originals`), the target `libs_capture` (the re-capture of
the originals run), `libs_test_wrong` (was `libs_emulator_tape_wrong`),
`libs_compare_*` (the compare checks, replace `libs_expect_fixed*`).

## 1. Measure and decide

- [ ] 1.1 Count the checks per program: run the originals test (needs the CD) with the program's own summary line (`NAME: N checks, M failures`) and write the 62 numbers to the task's commit message or a scratch file for D10. Verify: every program has a number
- [ ] 1.2 Re-run the address experiment of task 9.8 (a padded `libs_tests` binary and a heap shift) with a throwaway patch to list the observations that hold addresses; the list names the sites for `base+$OFF` or `prog`. Verify: the list is in the commit message of 2.2 and every site is converted in its group
- [ ] 1.3 Measure the text volume: write the observations of one mid-size program (`systest_mathstr`, `libtest ldivlib`) as lines with a throwaway patch and extrapolate the total (blocks for `systest_med` and `rwe_ridisasmlib` only, plain lines for the rest). Verify: the total is in the commit message; the user is told if it is unacceptable (design Risks)
- [ ] 1.4 List today's non-deterministic checks: run every program several times (at least 5) against the original, with the tape recording mode and with a different `LIBS_PAD`-like heap shift (throwaway patch), and list the checks whose observation differs between runs (the tape segment hashes or the summary lines show it). Verify: the list (program, check, reason) is in the commit message of 2.2; each entry has a planned treatment (fixed seed, defined state, `not captured: REASON`) in the group of its program

## 2. Infrastructure beside the tape

- [ ] 2.1 `libs/tests/capture.[ch]`: `cap_begin/cap_end`, line writer with a static buffer, number/bytes (hex up to 64 bytes, longer as length + FNV)/string/pointer formatters, `not captured`, block lines, `LIBS_CAP_DUMP` (named check or `BLOCK:N`). Verify: a host-compiled or emulator unit program (`libs_capture_format`) writes known lines (a 64-byte and a 65-byte buffer, a block) and the expected file text is compared
- [ ] 2.2 `LIBSET` and `LIBS_PAD` in `libs_tests`/`sysload.h`: `load_set` loads one file set; the padding allocation shifts the layout; unconverted programs keep `load_sets`. Verify: `ctest --preset all` unchanged
- [ ] 2.3 `capture-compare.cmake` with the exceptions table (`libs/tests/exceptions.tsv`, header as `tests/corpus/notes.tsv`; optional argument, absent for the originals test). Verify: the tests of D8 on fake files fail and pass as stated (`libs_compare_same`, `_value`, `_missing`, `_extra`, `_block`, `_longbuf`, `_row_pass`, `_row_stale_original`, `_row_stale_port`, `_row_missing_line`, `_row_ambiguous`, `_row_originals`, `_not_captured`)
- [ ] 2.4 `capture-accept.cmake` and the target `libs_capture`: two runs with `LIBSET=orig` and different `LIBS_PAD`, accept only if identical, else "test not deterministic" with the differing lines and nothing written; refuse a `lost` line; with `LIBSET=obj` write to the build directory and report the compare, no CD. Verify: `libs_accept_same`, `libs_accept_nondeterministic` (the committed directory stays unchanged), `libs_accept_lost` pass on fake files
- [ ] 2.5 `emulator.cmake`: `LIBSET`, the `captures/` output directory, the compare step after the run; wire into the existing emulator test (`obj`; renamed `libs_test` in 9.1) and the originals test (`orig`, no exceptions table). Verify: with one program converted (2.6) both tests pass and name its compare in their log
- [ ] 2.6 Convert `libtest.c` (ldivlib, lmullib) as the first program, plain lines; delete `libtest_*.tape`; commit its capture from `libs_capture` (originals). Verify: the C test passes without the CD; `libs_test_wrong` (changed line) fails with the line's message; a repeated `libs_capture` gives the same file

## 3. Framework macros

- [ ] 3.1 `sysload.h`: `OBS`, `OBSBYTES`, `OBSSTR`, one `struct set`, the stated-kept check with `kept`/`lost` lines (`KEEPS(routine, regs)`, D6), `check()` stays for the program's own consistency checks. Verify: `libs_accept_lost` (2.4) refuses a `lost` capture, and the emulator test `libs_test_lost` (a routine that loses a stated register, from a variant file with a broken thunk, gives `lost` and fails)
- [ ] 3.2 `rwe/rwecmp.h`: the same for `DO2`, `DO2S`, `preserve`, `KEEPS`, `rwe_done` (the summary table of kept registers stays as the aid for the stated sets). Verify: the first converted rwe program (4.1) compiles and passes

## 4. RWE userlibs A (alphabetical)

- [ ] 4.1 `bb_blitlib`, `blitzbomberslibtwo`, `ciatrklib` (with the determinism decision of D7 for the F00 phase), `neilsreqtoolslib`, `popupwinlib`. Verify: per program the number of checks equals task 1.1's, the capture is committed by `libs_capture` (which accepted it, so two layouts agree), `libs_test` passes, a stated kept set is the intersection of the original's calls
- [ ] 4.2 `riamosfunclib`, `rianimlib`, `riapplib`, `riarexxlib`, `riarrayslib`, `ribbmaplib`, `richunkylib`. Verify: as 4.1
- [ ] 4.3 `ricommoditieslib`, `ricompactdisclib`, `ricopperfxlib`, `ridatatypeslib`, `ridebuglib`, `ridisplaylib`, `ridoslib`. Verify: as 4.1
- [ ] 4.4 `ridisasmlib` with block lines (`cap_blocks`), its code buffer at the fixed address stays. Verify: as 4.1; a changed block hash fails naming the block (`libs_compare_block`); the capture file is smaller than 1 MB

## 5. RWE userlibs B

- [ ] 5.1 `riencryptlib` with the seeding or `not captured` decision of D7. Verify: as 4.1
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

- [ ] 7.1 Remove `selftest_fixed.c`, `expect_fixed`, `check_fixed`, `SAME_FIXED`, `fixed_verdict`; `libs_expect_fixed*` become the `libs_compare_row_*` tests of 2.3 (already present). Verify: `ctest --preset all -N | grep expect_fixed` is empty; the row tests pass
- [ ] 7.2 `libs_test_wrong` (changed line in an emulator run) for the C test and its counterpart for the originals test (`libs_test_originals_wrong`, a changed committed line fails the original's run too; needs the CD). Verify: both fail with the line message

## 8. Tapes out

- [ ] 8.1 Delete `tape.c`, `tape.h`, `tape-compare.cmake`, `tapes/`, `LIBSTAPE`/`LIBSTAPEDUMP`, the replay copy in `emulator.cmake`, `tape_*` calls and the `unconverted` branch of `load_sets`; delete `libs_emulator_tape_wrong`, `libs_tapes_compare_*`. Verify: `grep -rn "tape" libs/tests libs/acidlibs CLAUDE.md tests/README.md` finds no current mention (the archived change is history); `ctest --preset all` passes; a clean rebuild has no warnings
- [ ] 8.2 Acceptance: `ctest --preset all` and, with the CD, `ctest --preset originals` and `ctest --preset bug-orig`, plus `prev_results` (library files byte-identical). Verify: the three runs pass as before the change; the number of checks per program equals task 1.1's

## 9. Renames and documents

- [ ] 9.1 Rename `libs_emulator` to `libs_test` and `libs_emulator_originals` to `libs_test_originals` (the latter compares only; the capture is the target `libs_capture`) in `libs/tests/CMakeLists.txt`, `emulator.cmake`, the presets and every document. Verify: `grep -rn "libs_emulator" .` finds nothing current; `ctest --preset all -N` lists `libs_test`
- [ ] 9.2 `tests/README.md` "Libraries" and the presets table: single-library runs, capture format, `captures/`, the four operations (capture/test with each set), renewing a capture with `libs_capture` and reading its "test not deterministic" report, the debugging capture with `LIBSET=obj`, the exceptions table (C test only), the stated kept sets and their accepted limit, `not captured`, the rule "a fix does not change the scenario". Verify: the documented commands run as written
- [ ] 9.3 `CLAUDE.md` (names the tapes and `libs_emulator`), `libs/acidlibs/brexxlib.c` and the test programs' comments that name the tape. Verify: `grep -rn "tape" CLAUDE.md libs tests/README.md docs` shows nothing current
- [ ] 9.4 Sync the delta to the main specs (`openspec-sync-specs`) and archive after the acceptance. Verify: `openspec validate --specs` passes

## Out of scope

- Setup/act/verify asserts for the pure compute libraries: a later
  separate change, which can replace single captures by stated
  expectations.
- Libraries with no original (port-only) and checks beyond what the
  original does: the same later change.
