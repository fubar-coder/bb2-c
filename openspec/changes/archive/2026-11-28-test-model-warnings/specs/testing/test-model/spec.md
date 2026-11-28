# Delta: testing/test-model

## ADDED Requirements

### Requirement: Compiler warnings are a list of entries
Where a test of the model judges the warnings of a compile, its expected
output SHALL be the list of the warnings in the order the compiler gave
them, each with its place (the line of the test's own source where there
is one), its name and its text, and the return code of the compile where
the compiler is started from the command line.

#### Scenario: A warning that is missing
- **WHEN** a test expects one warning `datafrac` at line 11 of its source and the compile gives none
- **THEN** the case fails naming the query of the warnings, the expected entry and the empty list

#### Scenario: Two warnings in another order
- **WHEN** a test expects the warnings of lines 9 and 10 in that order and the compiler gives them as 10, 9
- **THEN** the case fails

### Requirement: The warnings are judged apart from other output
The list of the warnings SHALL be judged apart from the rest of the
compiler's output: a missing, an added or a changed entry and a changed
order SHALL each fail the case, and a change of other compiler output
SHALL NOT.

#### Scenario: Another message of the compiler
- **WHEN** the compiler prints a further line that is no warning and the warnings are those expected
- **THEN** the judgement of the warnings holds

#### Scenario: An added warning
- **WHEN** a test expects one warning and the compile gives two
- **THEN** the case fails naming the query of the warnings
