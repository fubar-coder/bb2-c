# Spec Delta

## Purpose

Makes the expected data of the tests belong to this repository, so that a
test says what is right and a deliberate difference from the original is
visible with its issue number instead of hidden by editing the test.

## ADDED Requirements

### Requirement: Corpus expected outputs are committed
Every corpus program SHALL have its expected output in `tests/corpus/expect/PASS/NAME.out` (and, for serial tests, the expected serial bytes in `NAME.serial`; one directory per pass, because a pass with runtime error checking on gives different outputs), generated once from the original's run and reviewed. The runner SHALL compare the output of every configuration with the file.

#### Scenario: Output differs
- **WHEN** a corpus program's output differs from its expect file in any configuration
- **THEN** the test fails and names the program, the configuration and the first differing line

#### Scenario: A fix changes an output
- **WHEN** a fix changes the correct output of a corpus program
- **THEN** the commit changes that expect file and names the issue; the program source is not edited to avoid the difference

### Requirement: Override rows keep the difference from the original visible
Where expected data is derived from the original (the CD result table `tests/data/blitz2-cd.tsv`, the TED oracle hashes `tests/data/ted-oracle.sha256`), a row holding the port's value for a fixed bug SHALL name the issue (`#NNN`) and the original's value, and the generator SHALL keep such rows when it regenerates the file.

#### Scenario: Regeneration
- **WHEN** the generator of the table or the hashes runs again
- **THEN** every row with an issue number keeps the port's value and the original's value, and no row loses its note

#### Scenario: A fix changes a CD source's result
- **WHEN** a compiler fix changes the executable of a CD source
- **THEN** the table update target rewrites that row with the new value, the issue and the original's hash, and the commit contains the change

### Requirement: Per-check fixed values in library pair tests
A `libs_emulator` check SHALL be able to replace "equal to the original" by a stated correct value together with the issue number (`expect_fixed(NNN, value)`); the scenario SHALL keep running against both libraries and the fixed check SHALL be asserted, not skipped. Scenarios SHALL NOT be edited to avoid a difference.

#### Scenario: A fixed check
- **WHEN** a library fix changes a result and the check has `expect_fixed(NNN, value)`
- **THEN** the C library must give `value`, the original may differ, and the other checks of the scenario still compare with the original

#### Scenario: Stale exemption
- **WHEN** an `expect_fixed` check finds that the C library gives the original's value
- **THEN** the test fails, so the exemption is removed with the fix that undoes it

### Requirement: Intended-difference lists
`refcheck data`, `rwe_declarations_NAME` and the `refcheck asm` tests SHALL accept an explicit list of intended differences with issue numbers, and each listed entry SHALL occur (a stale entry fails), as `blitzlib compare INTENDED` does. `ted_ab` scripts SHALL take a directive `; @fixed #NNN PAIR...` naming the combo pairs that must differ.

#### Scenario: Stale entry
- **WHEN** a list names a difference that no longer occurs
- **THEN** the test fails and names the entry

#### Scenario: Unlisted difference
- **WHEN** a difference occurs that no list names
- **THEN** the test fails
