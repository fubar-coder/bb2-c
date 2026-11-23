# Tasks

No original bug is fixed here (no rule-4 workflow); no result changes
(rule 3: no `PREV_INTENDED`). Each task runs the host tests of what it
touched and `ctest --preset host`; emulator tests as named.

## 1. Configuration and judging (host)

- [x] 1.1 `tools/lib/testconf`: the run settings `editor-run` (`off`, `on`; an axis like any single value) and `defaultdbug` (a path setting), the refusals of the spec (an assertion on `rc`/`requester`, `runwrap`/`runargs`/`launcherargs`/`pad = "on"` with `editor-run = "on"`, `defaultdbug` without it), each naming file, line and keys; `--print-config` shows both; verify: `testconf` and `testassert` host tests with broken input for each refusal
- [x] 1.2 The query `compile.inject` (`testassert.c` query table, `testjudge`): judged like `inject`, "not captured" without a compiler through inject, counted as a whole-program observation for the cases that run on their own; verify: `testjudge` host tests (captured, not captured, a case asserting it runs on its own)
- [x] 1.3 The defaults of an editor-run cell: `rc` and `requester` defaults not applied; a case with `!ExpectError` in such a cell refused at registration naming the line; verify: `testjudge` and `testreg_tool` host tests with broken input
- [x] 1.4 `@DRIVER@` in `jobline` replaced by the driver's source on the cell's volume, refused elsewhere; verify: `test_caserun` (the cell pass holds the job line), `testconf` (refused in `files`)

## 2. The cell pass and the session (runner)

- [x] 2.1 `cr_cell_pass`: `defaultdbug` staged in every cell it applies to (replacing `cd-defaultdbug` in the cells with original material), a missing module skips the side (77); `editrun=on` for an editor-run cell; `cr_prev_pass` drops `editrun`; verify: `test_caserun` (both sides, with and without the setting, the prev pass without `editrun` and `defaultdbug`)
- [x] 2.2 `corpus.h`/`corpus.c`: the key `editrun=on` (only with `cell=on`; refused with `runwrap=`, `runargs=`, `launcherargs=`): the job line `run=r001|CORPUS:src/NAME.bb2|CORPUS:out/NAME.con` behind the jobs, no shell run, the cell complete at `rc=` of `blitz2.out`, the hang check armed on the console file; verify: `test_corpus` (the job list and the script of such a cell, the refusals), and the measurement in 2.4
- [x] 2.3 `observe_cell`: `output` from `CORPUS:out/test.con` for an editor-run cell, `rc`/requesters not captured; `compile.inject` from `blitz2.out` when `wrap` is inject (`cr_inject_lines`); verify: `test_caserun` with a recorded cell directory of each kind
- [x] 2.4 Emulator self tests in `tests/blitz2/selftest` (or beside the model's self tests): an editor-run case that passes from its result file and its console output; one whose compiler runs through inject `track` and asserts `compile.inject`; a hang in an editor run (record when the session ends); verify: the three tests pass in `ctest --preset emulator -R <names>`, and the hang's end is noted in the design (D3)

## 3. The users move to the model

- [ ] 3.1 `libs/dhandlerlib-no-cache-flush` (#409) to `ID.test.bas`/`ID.test.toml` (D8); verify: both sides keep the verdict (port FIXED with the CD, skipped 77 without; originals as declared), fix-scope reports only the move
- [ ] 3.2 `regression/debuglib-setint-runtime-error` and `regression/compiler-macrobuff-nomem`; verify: `regression_*` tests pass on the side port, the old files removed in the same commit
- [ ] 3.3 `compiler/compiler-insasm-size-byte`, `-libsnode-name`, `-macro-textlen-max`, `-resconsts-last-next`, `-racing-stripe-wide`; verify: each keeps its verdict on both sides, the prev comparison (`-DBLITZ2_PREV=ON`) passes for them
- [ ] 3.4 `compiler/compiler-panic-remtask` only if the user's answer to Q1 asks for it (then a new task group for `editor-run = "before"` and the PANIC setting first); otherwise it stays in the old form and the U4 list names it; verify: the answer recorded in `rounds/tr/decisions.md`

## 4. Docs

- [ ] 4.1 `tests/README.md` ("The run configuration": the two keys in the table; "Assertions": `compile.inject` in the grammar and the table, the queries of an editor-run cell; "Moving a bug-orig test to the model": `jobline=run=` becomes `editor-run`, `defaultdbug=`, `[blitz2]` becomes `compile.inject`, a multi-line `compile.log contains` for `[log]`, `@DRIVER@`), `tests/corpus/runner/README.md`, the comment of `corpus.h`; verify: `basictest_readme` and the other README host tests pass, no text says the editor run is not in the model

## Workflow follow-up

- The coordinator removes the moved tests from the U4 list of `bug-orig-basic-tests` (classification file, class (b)) and runs `all`, `bug-orig` and `originals` once after the merge (D-010).
