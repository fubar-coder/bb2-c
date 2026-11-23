# Tasks

## 1. Preparation

- [x] 1.1 Show the user the wording of the fix workflow with `fixed-orig`/`fixed` for CLAUDE.md, `.claude/agents/tester.md`, `.claude/agents/implementer.md`, `docs/development.md` and `tests/README.md` (CLAUDE.md rule 1); verify: the user's OK recorded in `~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`
- [x] 1.2 Judging of declared deviations for the cells with the faulty part, the open-bug rule for the other cells, labels per case, the errors of `fixed-orig`/`fixed`, a test without a declared deviation as BROKEN; verify: host tests of each rule with broken input
- [x] 1.3 fix-scope for tests of the model (`tools/results fix-scope`): setting `fixed-orig = N` / `fixed = N` passes for issue N, other changes of checks, assertions or declarations are reported; verify: the scenarios of `testing/bug-fix-workflow` as host tests
- [x] 1.4 `bug-orig-record` skips tests of the model; verify: a test of the target with one test of each form
- [x] 1.5 The key `extension = N` for the test of an extension of the port (decided 2026-11-23, ~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md "Test rework step 1: prev comparison of model tests; extension key"): the requirement "The fixed issue is one key of three kinds" of the delta of `testing/bug-orig` and fix-scope's in `testing/bug-fix-workflow`; deviations required, judged and labelled like a fixed test, fix-scope's case `ID|extension||1 [fixed #N]`; verify: `openspec validate bug-orig-basic-tests --strict`, host tests of each rule with broken input (`extension` without a deviation, with `fixed-orig` or `fixed`, in a `tests.toml`)
- [x] 1.6 Move `tests/bug-orig/libs/shapeslib-loadshape-anim-interleave` (#1166, `; @fixed #1166/port`) to the model with `extension = 1166`, and drop the marker `/port` if no other test uses it; the wording of CLAUDE.md rule 4, `docs/development.md` and the agent files drafted for the user (CLAUDE.md rule 1), `tests/README.md` and `docs/` written; verify: the test passes on both sides, fix-scope counts its key as `[fixed #1166]`

- [x] 1.7 The side originals of an open test passes when its cells show exactly the declared deviations (decided 2026-11-23, ~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md "The originals side of an open bug passes"): `corpusrun case` gives exit status 0 for REPRODUCED on the side originals, the report saying "REPRODUCED (as declared, side originals)", for one case and for all cases of a source; the side port keeps exit status 1; verify: `openspec validate bug-orig-basic-tests --strict`, a host test (test_caserun: the deviation holds or not, on both sides), the nine `*_originals` tests that failed in the acceptance run of U1/U2 pass in `ctest --preset originals` and the port sides of the eight open ones still fail in `ctest --preset bug-orig`

## 2. Pilot round

- [x] 2.1 Move the five `shapeslib-*` tests of round R12's I4 (starting from the copies of spike 2) to `ID.test.bas` / `ID.test.toml` and a `tests.toml` for `tests/bug-orig/libs`; verify: each keeps its verdict with `ref`, `port` and `clib`, and their `.expect`/`.orig` are removed in the same commit
- [x] 2.2 Write the instructions for the movers from the pilot (D2 of design.md, with examples) into `tests/README.md`; verify: the coordinator's review of the pilot round (reviewed by the user 2026-11-23)

## 3. Rounds by area

- [x] 3.1 Plan the rounds; verify: `~/.local/share/bb2-c-work/decisions/rounds/tr/plan.md`, accepted by the user ("OK so", `~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`)
- [x] 3.2 Round U1, libraries graphics (~150 tests); verify: each moved test keeps its verdict on both sides, fix-scope run by the agents, `all`, `bug-orig` and `originals` run once by the coordinator after the merge
- [x] 3.3 Round U2, libraries system/IO/text (~135 tests); verify: as 3.2
- [x] 3.4 Round U3, libraries sound and compute (~45 tests); verify: as 3.2
- [ ] 3.5 Round U4, compiler tests with a program run, regression, selftest (~100 tests); verify: as 3.2
- [x] 3.6 Rounds U5-U7, RWE user libraries in three groups (~170 tests each); verify: as 3.2 per round

## 4. Closing

- [x] 4.1 Rename the header `; @fixed #N` of the tests left in the old form (TED, compile-only) to `; @fixed-orig #N`, with the runner, fix-scope, `check-fixed-orig.cmake` and the texts of D5 (wording shown to the user first); verify: `ctest --preset all` and `bug_orig_fixed_orig_broken` pass, no `; @fixed #` left in `tests/bug-orig`
- [x] 4.2 Update `tests/README.md`, `docs/testing.md`, the test statistics and TODO.md ("Test model beyond the library tests") to the new state; verify: no text describes a moved test with `.expect`/`.orig`
