# Tasks

Starts after the user has answered the proposal's open questions 1 and 2. With another answer than (a) the coordinator has the planning agent update this change (openspec-update-change) before task 1. An enhancement, not an original bug: no bug-orig test, no `Fixes #N`; the commit names `#1172`.

## 1. Preparation

- [ ] 1.1 Spike in the emulator (no commit): a scratch internal library with the two checks and the init of design decisions 3 and 4, embedded in a scratch blitz2-ab3, and a hand-made program compiled with `-r`: the probe of the default handler with output on the console, after WindowOutput, after FileOutput and DefaultOutput, at a channel stack depth of 15; return code 20 by the stub, with the stack level from `cbreakinit` and from an init entry; a program without the switch byte-identical with the library embedded. Also the library number 64029 and its token range: free in every `libs/variants/*/libnums.h`, the CD's lists, AmiBlitz3's `libnums.ab3`/`sorted_libnums.ab3`, the library files under `archive/` and AmiBlitz3's binary libraries. Verify: the report gives each case's result and the chosen init form; on a failure stop and report to the coordinator (no fallback of its own).

## 2. Tests first

- [ ] 2.1 Host test `tools/tests/test_ab3args.cpp`: `-ctrlcbreak`, `-CTRLCBREAK`, absent, given twice; the usage lists it. Verify: fails on main.
- [ ] 2.2 `tests/ab3/cli` cases (ARGS `-r -ctrlcbreak` unless named; no oracle expectation, AmiBlitz3 has no such option): `cbreak_print` (NPrint after `SetSignal_ $1000,$1000`: "***Break", return code 20, the next line not printed), `cbreak_file` (FileOutput: goes on; DefaultOutput: ends), `cbreak_edit` (Edit$ in an expression from redirected input: the line read, then the end), `cbreak_consumed` (`SetSignal_ 0,$1000` before the Print: goes on), `cbreak_cleanup` (a file opened before the break is closed), `cbreak_runerrs` (ARGS `-ctrlcbreak` without `-r`, runtime error checking from the source: as #43; and a Print between `Runerrsoff` and `Runerrson`: ends), `cbreak_ab3` (an `.ab3` source: refused, return code 20, no executable), `cbreak_off` (`-r` only: goes on). Verify: they fail on main (unknown option); `rundebuglib-ctrlc-*` unchanged and passing.

## 3. Implementation

- [ ] 3.1 The option (`shared/ab3job/ab3job.h`, `ab3args.c`): the job byte, the parse, the usage (design decision 1). Verify: 2.1 passes.
- [ ] 3.2 The internal library (next to `libs/compilerlib/`, its CMake entry, the embedding in `asmlibs.c` `loadlibs` as `compilerlib`'s): the two checks and the init (design decisions 3, 4, 6); `blitzlib check` clean, no warnings. A library test case in `libs/tests` (without the signal: registers kept, no OS call; with the signal and a non-default handler: registers and signal kept). Verify: `libs_tests` pass.
- [ ] 3.3 The compiler (`ext.h`, `ext_bb2.c`, `ab3main.c` `ext_ctrlcbreak` and the AB3-mode refusal; `libcall.c` `makelsub3` on its `.norerr` path; `main.h`): design decisions 2, 5. `eval.c` is not edited. Verify: the cases of 2.2 pass; `git diff --stat` lists no `eval.c`.

## 4. Checks

- [ ] 4.1 `ctest --preset host`; `ctest --preset all -R '^ab3_cli|blitz2_|regression_|libs_tests|corpus$|corpus_runerr'`. Verify: pass.
- [ ] 4.2 Prev comparison: no library file and no executable differs (TED, blitz2 and blitz2-ab3 without the switch; the corpus, the CD, `tests/ab3/cli` cases without it). Verify: `ctest --preset all -R prev` without `-DPREV_INTENDED` entries for results (the programs blitz2 and blitz2-ab3 are named as reworked in the commit).
- [ ] 4.3 Cost: a `tests/perf` line for NPrint to NIL with and without the switch (the check without the signal: the test of `tc_SigRecvd` and an `rts`). Verify: `ctest --preset perf`; the report gives the cycles; no existing budget rises.

## 5. Documents

- [ ] 5.1 `docs/ab3/usage.md` (the option table and a paragraph: what it covers, AB3 mode refused, TED has no such option yet), `docs/blitz2/extensions.md`, the library's head comment, the comment at `makelsub3`; the site: `ab3-mode/command-line.md` (the option), `compared-with-original/extensions.md` (a section), a sentence and a link in the #43 entry of the fixed-bugs page. Verify: coordinator's review; links resolve.

## 6. Commit and report

- [ ] 6.1 One commit for the option, the library and the compiler with the tests; one site commit. The message names `#1172`, says that no result changes and names the reworked programs blitz2 and blitz2-ab3. Report: the spike's results, the measured cost, the files. Verify: `git show --stat`.
