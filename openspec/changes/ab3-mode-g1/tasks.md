# Tasks

Every commit: one topic, English texts; `blitz2`, `ted`, `acidlibs`,
`deflibs` and all library files byte-identical to the previous commit
(hash comparison, `*/tests/*` excluded); clean rebuild without
warnings; `ctest --preset host`.

## 1. Test data and the second target

- [x] 1.1 Add AmiBlitz3 v3.14.0's files to `cmake/fetch-originals.cmake` (the four BigLibs, `Residents/all.res`, the `Amiblitz3` binary, the test sources of the spike's set, from GitHub raw at `58a11658` with SHA-256, into ignored `archive/amiblitz3/github-58a11658/`) and document them in `archive/README.md`; verify: `--target fetch-originals` downloads them and a changed hash fails
- [x] 1.2 Build `blitz2-ab3` from the same sources with `-DBLITZ2_AB3` (one CMake definition for both targets, the link check of `checklink.cmake` for both); verify: `blitz2` byte-identical, `blitz2-ab3` builds without warnings, and the size of its globals is checked against the 64 KB of `-fbaserel` (if exceeded: `BLITZ2_BASEREL=OFF` for `blitz2-ab3`, noted in the commit)
- [x] 1.3 Add a ctest that compiles the corpus and bug-orig sources with `blitz2-ab3` through `tedstub` and compares every executable with `blitz2`'s; verify: it passes now, and fails on a deliberately changed `blitz2-ab3` (spec `ab3-compat/bb2-equivalence`)

## 2. Mode detection

- [x] 2.1 Recognise AmiBlitz3's 15-entry `.xtra` format in `shared/xtra` (formats.md §1.3) and extend the `xtra` host tests (AB3 `.xtra`, BB2 `.xtra`, truncated input); verify: `ctest --preset host`
- [x] 2.2 Implement the mode rule (option, embedded header, `.ab3`, companion `.xtra`) as a C module usable by `blitz2-ab3` and on the host, with a host test of every scenario of spec `ab3-compat/source-mode` "recognised"/"forced"; verify: host test passes
- [x] 2.3 Check how blitz2 compiles `INCLUDE`/`XINCLUDE` of text and of tokenised files (`tokens1.c`), record the finding in `design.md` D4, and add the "Include of another mode: NAME" error for tokenised includes of the other mode under `#if BLITZ2_AB3`; verify: `blitz2` byte-identical, the error test of task 5.3

## 3. Command line and `ab3cli`

- [x] 3.1 Parse the command line in `blitz2-ab3` (`-s/-source`, `-e/-exe`, `-r/-release`, `-mode`, `-h/-help`; refuse AmiBlitz3's other options with return code 20) as a module with a host test of spec `ab3-compat/command-line` (valid, missing argument, unknown and unsupported options, usage); verify: host test passes, no `-s` → editor start unchanged
- [x] 3.2 Build `ab3cli` on the edstub frame with TED's core (text loading/tokenising) and `shared/xtra`; it gets the job from `blitz2-ab3`, loads the source, hands options and residents as a BB2 `.xtra` image (`-r`: runtime errors and debug info off), compiles with CREATE EXECUTABLE and prints errors as "FILE:LINE: MESSAGE"; return codes 0/10/20; verify: the checks of the edstub programs (hunkcheck), and task 5.2
- [x] 3.3 Load `PROGDIR:ab3cli` instead of `blitz2:ted` when `blitz2-ab3` runs with `-s`, after detecting the mode; verify: `blitz2-ab3 -s` of a BB2 tokenised corpus source gives the same executable as `blitz2` through `tedstub`
- [x] 3.4 Document the command line in README (section on blitz2-ab3) and `docs/ab3/variant.md`; verify: the documented example runs in the emulator as written
- [x] 3.5 Tokenise text includes in a command line run (D4: blitz2 only reads tokenised includes): in `ab3cli` runs, `readinc` has a text include tokenised by the editor with the program's token list; editor sessions keep `readinc` unchanged; verify: an AB3 program and a BB2 program (both from the command line) with a text include compile and run with the expected output (spec `ab3-compat/source-mode` "Text include …"), `blitz2` byte-identical, `ab3_bb2_equivalence` passes
- [x] 3.6 A compile error inside an include is reported with the include's file name and its line (today: the main source's line); verify: a test of task 5.3 with an error in a text include and one in a tokenised include, `blitz2` byte-identical, `ab3_bb2_equivalence` passes

## 4. AB3 libraries, tokens, residents

- [x] 4.1 Add AmiBlitz3's compiler token chain under `#if BLITZ2_AB3` (D3, from `AmiBlitz3.ab3` at `58a11658`, referenced as `AmiBlitz/AmiBlitz3@58a11658`) and select the chain by mode; the new tokens $80D4–$80DB give a "not supported" compile error; verify: the token list test of task 4.4
- [x] 4.2 Load the library set of the mode (D5): the four BigLibs from `Blitz3:BlitzLibs/` in AB3 order, a missing file stops with an error naming it, the flat `UserLibs/` directory with replacement of a library of the same number; verify: `blitz2` byte-identical; tests of task 5.3 (missing library, userlib replacing)
- [x] 4.3 Make `mathq` per mode (192 / 44) and prefix resident names without path with `Blitz3:BlitzLibs/Residents/` in AB3 mode; verify: `blitz2` byte-identical; the `^` and resident tests of task 5.2
- [x] 4.4 Add the token list test: the AB3-mode token list of `blitz2-ab3` with v3.14.0's BigLibs (through `blitztokens` with the AB3 compiler chain) equals AmiBlitz3's own (`-dump token` in the emulator, or the list derived from AB3's binary); a changed list must fail; verify: ctest passes, broken input fails

## 5. Emulator tests of AB3 programs

- [x] 5.1 Set up a test suite `ab3` (label `emulator`, return 77 without Amiberry or the fetched files): a volume with `blitz2-ab3`, `ab3cli`, `Blitz3:` with the fetched AB3 files, programs compiled with `blitz2-ab3 -s … -e …` and run, output compared with `.expect` files; verify: a deliberately wrong expect file fails
- [x] 5.2 Add the spike's AB3 test programs (`atoi`, `compilersignals`, `data_restore_in_function`, `exec_data`, `localization`, `fabrizio_speedquestion`, `memoryleak_test` without its `OPTIMIZE` line, `setint`, `test_newtypes`, `trim`) plus a `^` test and a header-resident test; verify: all pass
- [x] 5.3 Add the error tests: tokenised include of the other mode (both directions), missing library file, missing resident, unsupported option, compile error output and return code; verify: all pass
- [x] 5.4 Add the `ref` configuration: the same programs compiled with AmiBlitz3 v3.14.0's own compiler in CLI mode, outputs compared with `blitz2-ab3`'s; verify: the comparison passes for every program, differences are recorded in `docs/ab3/variant.md` as G2+ items

## 6. Integration

- [x] 6.1 Full check (including the CD sources compiled with `blitz2-ab3` against `tests/data/blitz2-cd.tsv`, as for `blitz2`): clean rebuild without warnings, `blitz2` byte-identical to the commit before the change, `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts; update `docs/ab3/variant.md` (G1 done, measured share of AB3 test programs that compile) and `TODO.md`

## Workflow follow-up

- Archive the change after review (`/opsx:archive`), syncing the specs into `openspec/specs/ab3-compat/`.
