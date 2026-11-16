# testing/original-comparisons Specification

## Purpose
States what the tests still compare with the originals once expected
results are the rule, so that the comparisons that define "BB2" remain and
those that would block fixes are bounded.

## Requirements

### Requirement: Label originals
Every test that needs the CD or the reference build (live reference configurations of the corpus and `ted_ab`, `libs_emulator_originals`, `rwe_refbuild*`, `rwe_residents_cd`, `rwe_lesfont_cd`, `tokens_cd`, the compare test `bug_orig_record_ID` of the bug-orig tests, the CD result table and TED oracle generators) SHALL carry the label `originals` and a preset SHALL select it. The compare test of a bug-orig test SHALL be the detector of a changed original and of a recording that is not deterministic: it SHALL compare the original's observation with the committed `ID.orig`, for open and for fixed tests, and SHALL write nothing. The label `oracle` SHALL keep meaning AmiBlitz3's compiler checks of `tests/ab3`.

#### Scenario: all without the CD
- **WHEN** `ctest --preset all` runs on a machine without the CD
- **THEN** no test is skipped for a missing reference, except those that read the CD's source files as input (`corpus_cd`, the TED core oracle tests)

#### Scenario: Preset originals
- **WHEN** `ctest --preset originals` runs with the CD
- **THEN** it validates the committed expected data and recorded originals against the originals and writes no file

#### Scenario: The original compiler with the port's libraries
- **WHEN** `ctest --preset originals` runs a corpus pass
- **THEN** its configuration `orig-clib` (the original blitz2 2.10 with the build's acidlibs and deflibs) is judged against the expected data of the pass (the unqualified case and, for a fixed program, either of its `[orig]` and `[fixed #N]` files), and a program whose output matches none fails

### Requirement: BB2 definition stays compared
The declarations of acidlibs and deflibs (`blitzlib compare` with the intended list), the token list (`tokens_port`) and the AB3 comparisons SHALL keep comparing with their definition: the CD, the original TED's list and AmiBlitz3. A fix SHALL NOT add or rename a command.

#### Scenario: A fix changes a declaration
- **WHEN** a fix changes a register byte of a declaration
- **THEN** the intended list names the entry with the issue, and without the entry the comparison fails

### Requirement: Routine-wise instruction comparison
`asmcmp` tests SHALL compare only routines still equal to the original; a fix inside a listed routine SHALL remove the routine from the list in the same commit.

#### Scenario: Fix in a listed routine
- **WHEN** a fix changes a routine of a library listed in an `asmcmp` test
- **THEN** the commit removes the routine from the list and the test passes

### Requirement: Executables are not compared with the original
A test that compares the port with the original (label `originals`: the corpus passes `*_originals`, the CD result table, `ted_ab_originals`) SHALL compare outputs and errors: the output and serial data of a program, whether a source compiles, the error line and message of one that does not, the files TED writes. The hash and size of an executable the port makes SHALL be reported where the comparison sees it and SHALL NOT make the test fail. A fix or an optimization that changes an executable but no output or compile result SHALL need neither an issue nor expected data for the comparison with the original. Comparisons with the port's own earlier state (`prev` build, `PREV_INTENDED`) and between the port's two compilers (`tests/ab3`) SHALL stay byte exact.

#### Scenario: Different executable, same output
- **WHEN** the port's executable of a corpus program differs from the original's and the output is as expected
- **THEN** `NAME_originals` passes and its report names the difference as informational

#### Scenario: Different output
- **WHEN** the port's output of a program differs from the expected data
- **THEN** the test fails, as before

#### Scenario: A source the port no longer compiles
- **WHEN** the original compiles a source and the port does not (or the error line differs)
- **THEN** the comparison fails; a fix that changes this carries a `#N:` note in the CD table

#### Scenario: Prev stays exact
- **WHEN** the executable of a corpus program differs from the `prev` build's without `PREV_INTENDED`
- **THEN** the test fails
