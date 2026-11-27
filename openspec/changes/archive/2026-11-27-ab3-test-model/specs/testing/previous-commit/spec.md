## MODIFIED Requirements

### Requirement: The tests of the BASIC test model are compared with the prev build
With a `prev` build, a test of the model in `tests/blitz2` or
`tests/ab3/model`, and a bug-orig test of the model whose subject is the
compiler, SHALL have the driver of all its cases compiled by the current
and by the previous commit's compiler (blitz2-ab3 for `compiler = "ab3"`),
once for each value of runtime error checking among its cells on the side
port. The executables and compile results SHALL be the same unless the
difference is named in `PREV_INTENDED`.

#### Scenario: Unchanged rework
- **WHEN** a commit changes no executable and the comparison of a test of the model runs
- **THEN** it passes

#### Scenario: Unintended change
- **WHEN** the executable of a test's driver with runtime error checking differs from the one the previous commit's compiler writes and no entry names it
- **THEN** the comparison fails naming `ID-runerrs`

#### Scenario: A test of AB3 mode
- **WHEN** a test of `tests/ab3/model` with the cells `ab3` and `amiblitz3` is compared
- **THEN** its driver is compiled by both commits' blitz2-ab3 and AmiBlitz3 is not run
