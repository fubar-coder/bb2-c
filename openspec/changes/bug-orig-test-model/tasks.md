# Tasks

Each task leaves `ctest --preset host` green (and `ctest --preset all`
where it touches the emulator tests); a task with a check has a test with
deliberately broken input.

## 1. Case projection and acceptance (host side)

- [x] 1.1 Re-read the fix-scope interface of `tools/lib/libscmp` on main (`libscmp_parse`, `libscmp_issues_*`, `libscmp_fix_scope`; the change `libs-capture-host-blocks` may have changed `libscmp.h`) and note differences in the design. Verify: the design's D6 names only functions that exist.
- [x] 1.2 Add `tools/lib/bugorigcmp` with the projection of a test (`ID.expect`, `ID.orig`, header `@fixed #N`, TED directive) onto a `libscmp` case file, per D6. Verify: host test `tools/tests/test_bugorigcmp.c` from memory (open test, fixed test, long section, TED sections, a test without `ID.orig`).
- [x] 1.3 Add the acceptance of two observations (equal: result; different: "test not deterministic: ID" with sections and lines) and the wildcard-keeping rewrite of D4, plus `results bug-orig-accept RUN1 RUN2 OUT`. Verify: host tests for equal runs, one differing section, a wildcard kept, a wildcard that does not match both runs (reported, nothing written).

## 2. Wildcards with a reason

- [x] 2.1 The runner (`tests/corpus/runner/bugorig.c`) reads a wildcard line only after a `# varies: REASON` line, else exit status 2 with file and line; the same for the TED mode (`tests/ted-ab/runner/run.c`). Verify: extend `tests/corpus/runner/test_bugorig.c` and a ctest on a fake `ID.orig` (`bug_orig_wildcard_noreason`, expected failure message).

## 3. Compare-only record test and recording target

- [x] 3.1 Replace the environment variable `BUGORIG_RECORD=1` by the pass key `record=check` (compare, no file written) in `tests/corpus/runner/cmd_pass.cpp`/`corpus.h`; `tests/bug-orig/CMakeLists.txt` passes it for `bug_orig_record_ID` (Blitz, `:315-319`). Verify: `ctest --preset originals -R bug_orig_record_` leaves `git status` clean; a ctest on a fake test with a changed `.orig` fails naming the line (`bug_orig_record_changed`, label `originals`) and a host test proves that the mode opens no file for writing.
- [x] 3.2 The same for the TED tests (`tests/ted-ab/runner/run.c`, `tests/bug-orig/CMakeLists.txt:558-564`). Verify: as 3.1.
- [x] 3.3 Add the target `bug-orig-record` for Blitz tests: key `record=write`, two runs of `ref` (the second with `inject.c` `pad`), the acceptance of 1.3, `FILTER=ID`. Verify: `cmake --build --preset amiga --target bug-orig-record` with `FILTER` on two tests leaves their files byte-identical to main and the other files untouched; a fake run with a differing second run prints "test not deterministic" and writes nothing (ctest on the accept step with fake files).
- [x] 3.4 Check whether `tedab` can vary the layout of the `ref` run (for example `BB2:` mounted or not, the `pad` of the stub); extend `bug-orig-record` to TED tests, with the layout variation where possible, otherwise two identical runs and the limit stated in the docs. Verify: `FILTER` on one TED test reproduces its file.

## 4. The four varying files

- [ ] 4.1 Give `blitzcoplib-maxslice-zero`, `bb_blitlib-createshape-count0`, `rigfxlib-cpucls-no-plane-crash` and `rishapeslib-read-palette-colour-count` (#820) a wildcard line with `# varies: REASON` where recording twice differs (find the line by recording each three times with `bug-orig-record`; where a whole section varies, apply D5). Verify: `ctest --preset originals -R bug_orig_record_` passes three times in a row with no change in `git status`, and `ctest --preset bug-orig` verdicts of the four stay REPRODUCED.

## 5. Fixed tests keep `ID.orig`

- [ ] 5.1 Record `ID.orig` of every fixed bug-orig test (one `bug-orig-record` run, reviewed with `git diff`, files only added); the validity check (BROKEN) also runs on them. Verify: `ctest --preset all` and `ctest --preset bug-orig` unchanged in verdicts; no `ID.orig` equals its `ID.expect` (BROKEN would show).
- [ ] 5.2 Replace `tests/bug-orig/check-fixed-orig.cmake` by the opposite check (a fixed test must have `ID.orig`), with its test `bug_orig_fixed_orig_broken` turned around (fake tree `tests/bug-orig/selftest/fixed-orig/`). Verify: configuring a tree with a fixed test without `ID.orig` fails with the message; with it, passes.
- [ ] 5.3 A fixed test gets its compare test `bug_orig_record_ID` (label `originals`, as the open tests) in the Blitz and TED branches of `tests/bug-orig/CMakeLists.txt`. Verify: `ctest --preset originals -N` lists one per bug-orig test; `ctest --preset all -N` lists none of them.

## 6. fix-scope covers bug-orig

- [ ] 6.1 Extend `cmake/fix-scope.cmake` and `results fix-scope` with the bug-orig tests (the projection of 1.2 for base and branch, then `libscmp_fix_scope_dirs`). Verify: extend `fix_scope_*` host tests: a branch with `Fixes #7` adding `; @fixed #7` and a changed `ID.expect` passes; a branch that also changes another test is reported with its keys; a branch with no `Fixes` line reports every changed case; a malformed test file fails.
- [ ] 6.2 The target `fix-scope` runs both on a throwaway git repository (as `fix_scope_target` does). Verify: the existing `fix_scope_target` test plus one bug-orig case.

## 7. Acceptance

- [ ] 7.1 Full run: `ctest --preset all`, `ctest --preset bug-orig`, `ctest --preset originals` (with the CD), `ctest --preset host` with `-DHOST_SANITIZE=ON`; the `prev` comparison shows executables, TED files and library files byte-identical. Verify: the runs pass; `git status` after `ctest --preset originals` is clean.

## 8. Documents

- [ ] 8.1 `docs/testing.md`: suites table (bug-orig row: expected data `ID.expect`, recorded original `ID.orig`, a fix is `; @fixed #N`; both the pair), remove bug-orig from "How they differ" and "Planned follow-ups", the walkthrough of a fix, the one for "test not deterministic" (the bug-orig target). Verify: the text agrees with the specs; links resolve.
- [ ] 8.2 `tests/README.md`: the bug-orig sections ("Recorded originals", "Fixed tests", adding a test), the `originals` preset line (no recording in it), `.orig` of fixed tests, the wildcard rule; `docs/development.md`, `CLAUDE.md` and `.claude/agents/tester.md` where they name the recording test. Verify: `grep -rn "BUGORIG_RECORD" .` finds no use as environment variable of a test; the rules agree (spec `testing/bug-fix-workflow` "Rules agree").
- [ ] 8.3 Site: `site/content/docs/project-structure/tests.md` and `site/content/docs/compared-with-original/comparing.md`: what the recorded original is and that it is kept after a fix. Verify: `hugo` via podman builds the pages and they were looked at.
- [ ] 8.4 `TODO.md`: the follow-up entry done (date), `tools/testinfo/rules.txt` checked (the `.orig` rule). Verify: no strikethrough; `rules.txt` still selects the bug-orig tests for `*.orig`.
