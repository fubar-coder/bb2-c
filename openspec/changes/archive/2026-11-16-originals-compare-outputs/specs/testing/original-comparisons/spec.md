## ADDED Requirements

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
