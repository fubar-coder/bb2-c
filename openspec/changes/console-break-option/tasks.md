# Tasks

The user's decisions 1-9 are in the proposal; no question is open. An enhancement, not an original bug: no bug-orig test, no `Fixes #N`; the commit names `#1172`.

## 1. Preparation

- [ ] 1.1 Spike in the emulator (no commit): inputoutputlib with the two entries and the recorded stack level (design decisions 3, 4), a hand-made program that calls them after an NPrint (compiled with `-r`): return code 20 and the finits run; the stack level of inputoutputlib's init equals runerrlib's. Verify: the report gives the results; on a failure stop and report to the coordinator.

## 2. Tests first

- [ ] 2.1 Host test `tools/tests/test_ab3args.cpp`: `-ctrlcbreak`, `-CTRLCBREAK`, absent, given twice; the usage lists it. Verify: fails on main.
- [ ] 2.2 `tests/ab3/cli` cases (ARGS `-r -ctrlcbreak` unless named; no oracle expectation, AmiBlitz3 has no such option): `cbreak_print` (NPrint after `SetSignal_ $1000,$1000`: "***Break", return code 20, the next line not printed), `cbreak_file` (FileOutput: goes on; DefaultOutput: ends), `cbreak_edit` (Edit$ in an expression from redirected input: the line read, then the end), `cbreak_consumed` (`SetSignal_ 0,$1000` before the Print: goes on), `cbreak_cleanup` (a file opened before the break is closed), `cbreak_runerrs` (ARGS `-ctrlcbreak` without `-r`, runtime error checking from the source: as #43; and a Print between `Runerrsoff` and `Runerrson`: ends), `cbreak_ab3` (an `.ab3` source: the note, the executable written, it equals the one without the switch, the return code as without the switch), `cbreak_oldlib` (BB2 mode with the CD's acidlibs, whose inputoutputlib lacks the entries: the note, the executable equals the one without the switch, the same return code; label `originals`, skipped without the CD), `cbreak_off` (`-r` only: goes on). Verify: they fail on main (unknown option); `rundebuglib-ctrlc-*` unchanged and passing.

## 3. Implementation

- [ ] 3.1 The option (`shared/ab3job/ab3job.h`, `ab3args.c`): the job byte, the parse, the usage (design decision 1). Verify: 2.1 passes.
- [ ] 3.2 inputoutputlib (`libs/acidlibs/inputoutputlib.c`): `cbreakout` (+15) and `cbreakin` (+16) after `init`, the stack level in its init, the end (design decisions 3, 4, 6); the head comment's entry list; `blitzlib check` clean, no warnings. A library test case in `libs/tests` (without the signal: registers kept, no OS call; with the signal and a non-default handler: registers and signal kept). Verify: `libs_tests` pass (`libs-update` where inputoutputlib's capture changes, reviewed with `git diff`).
- [ ] 3.3 The compiler (`ext.h`, `ext_bb2.c`, `ab3main.c` `ext_ctrlcbreak` and the notes; `libcall.c` `makelsub3` on its `.norerr` path; `main.h`): design decisions 2, 5. `eval.c` is not edited. Verify: the cases of 2.2 pass; `git diff --stat` lists no `eval.c`.

## 4. Checks

- [ ] 4.1 `ctest --preset host`; `ctest --preset all -R '^ab3_cli|blitz2_|regression_|libs_tests|corpus$|corpus_runerr'`. Verify: pass.
- [ ] 4.2 Prev comparison: the differing results are inputoutputlib's file, the `acidlibs` files and the executables that link inputoutputlib (linked whole; #1196 is to remove this growth), nothing else; their list into `PREV_INTENDED` and the commit message, with the growth in bytes per executable. Verify: `ctest --preset all -R prev` with `-DPREV_INTENDED`.
- [ ] 4.3 Cost: a `tests/perf` line for NPrint to NIL with and without the switch (the check without the signal: the test of `tc_SigRecvd` and an `rts`); the existing budgets of Print and Edit unchanged without the switch. Verify: `ctest --preset perf`; the report gives the cycles.

## 5. Documents

- [ ] 5.1 `docs/ab3/usage.md` (the option table and a paragraph: what it covers, AB3 mode and libraries without the entries ignored with a plain note, the return code unchanged, TED has no such option yet), `docs/blitz2/extensions.md`, `docs/libs/deviations.md` (inputoutputlib's two entries; not kind F), the comment at `makelsub3`; the site: `ab3-mode/command-line.md` (the option), `compared-with-original/extensions.md` (a section), a sentence and a link in the #43 entry of `compared-with-original/libraries/commands/rundebuglib.md`. Verify: coordinator's review; links resolve.

## 6. Commit and report

- [ ] 6.1 One commit for the option, inputoutputlib and the compiler with the tests and `PREV_INTENDED`; one site commit. The message names `#1172`, the changed results (user decision 7) and the reworked programs blitz2 and blitz2-ab3. Report: the spike's results, the measured cost and growth, the files. Verify: `git show --stat`.
