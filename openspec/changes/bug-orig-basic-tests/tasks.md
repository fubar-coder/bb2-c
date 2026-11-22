# Tasks

## 1. Preparation

- [x] 1.1 Show the user the wording of the fix workflow with `fixed-orig`/`fixed` for CLAUDE.md, `.claude/agents/tester.md`, `.claude/agents/implementer.md`, `docs/development.md` and `tests/README.md` (CLAUDE.md rule 1); verify: the user's OK recorded in `~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`
- [x] 1.2 Judging of declared deviations for the cells with the faulty part, the open-bug rule for the other cells, labels per case, the errors of `fixed-orig`/`fixed`, a test without a declared deviation as BROKEN; verify: host tests of each rule with broken input
- [x] 1.3 fix-scope for tests of the model (`tools/results fix-scope`): setting `fixed-orig = N` / `fixed = N` passes for issue N, other changes of checks, assertions or declarations are reported; verify: the scenarios of `testing/bug-fix-workflow` as host tests
- [ ] 1.4 `bug-orig-record` skips tests of the model; verify: a test of the target with one test of each form

## 2. Pilot round

- [ ] 2.1 Move the five `shapeslib-*` tests of round R12's I4 (starting from the copies of spike 2) to `ID.test.bas` / `ID.test.toml` and a `tests.toml` for `tests/bug-orig/libs`; verify: each keeps its verdict with `ref`, `port` and `clib`, and their `.expect`/`.orig` are removed in the same commit
- [ ] 2.2 Write the instructions for the movers from the pilot (D2 of design.md, with examples) into `tests/README.md`; verify: the coordinator's review of the pilot round

## 3. Rounds by area

- [x] 3.1 Plan the rounds; verify: `~/.local/share/bb2-c-work/decisions/rounds/tr/plan.md`, accepted by the user ("OK so", `~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`)
- [ ] 3.2 Round U1, libraries graphics (~150 tests); verify: each moved test keeps its verdict on both sides, fix-scope run by the agents, `all`, `bug-orig` and `originals` run once by the coordinator after the merge
- [ ] 3.3 Round U2, libraries system/IO/text (~135 tests); verify: as 3.2
- [ ] 3.4 Round U3, libraries sound and compute (~45 tests); verify: as 3.2
- [ ] 3.5 Round U4, compiler tests with a program run, regression, selftest (~100 tests); verify: as 3.2
- [ ] 3.6 Rounds U5-U7, RWE user libraries in three groups (~170 tests each); verify: as 3.2 per round

## 4. Closing

- [ ] 4.1 Rename the header `; @fixed #N` of the tests left in the old form (TED, compile-only) to `; @fixed-orig #N`, with the runner, fix-scope, `check-fixed-orig.cmake` and the texts of D5 (wording shown to the user first); verify: `ctest --preset all` and `bug_orig_fixed_orig_broken` pass, no `; @fixed #` left in `tests/bug-orig`
- [ ] 4.2 Update `tests/README.md`, `docs/testing.md`, the test statistics and TODO.md ("Test model beyond the library tests") to the new state; verify: no text describes a moved test with `.expect`/`.orig`
