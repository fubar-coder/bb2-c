# testing/original-comparisons Specification

## Purpose
States what the tests still compare with the originals once expected
results are the rule, so that the comparisons that define "BB2" remain and
those that would block fixes are bounded.

## Requirements

### Requirement: Label originals
Every test that needs the CD or the reference build (live reference configurations of the corpus and `ted_ab`, `libs_emulator_originals`, `rwe_refbuild*`, `rwe_residents_cd`, `rwe_lesfont_cd`, `tokens_cd`, the recording run of the bug-orig tests, the CD result table and TED oracle generators) SHALL carry the label `originals` and a preset SHALL select it. The label `oracle` SHALL keep meaning AmiBlitz3's compiler checks of `tests/ab3`.

#### Scenario: all without the CD
- **WHEN** `ctest --preset all` runs on a machine without the CD
- **THEN** no test is skipped for a missing reference, except those that read the CD's source files as input (`corpus_cd`, the TED core oracle tests)

#### Scenario: Preset originals
- **WHEN** `ctest --preset originals` runs with the CD
- **THEN** it validates the committed expected data against the originals

#### Scenario: The original compiler with the port's libraries
- **WHEN** `ctest --preset originals` runs a corpus pass
- **THEN** its configuration `orig-clib` (the original blitz2 2.10 with the build's acidlibs and deflibs) is judged against the expected files, and a program that differs without a row in `tests/corpus/notes.tsv` fails

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
