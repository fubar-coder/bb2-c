# Spec Delta

## ADDED Requirements

### Requirement: The tests of the BASIC test model are compared with the prev build
With a `prev` build, a test of the model of `testing/basic-test-runner` in
`tests/blitz2`, and a bug-orig test of the model whose subject is the
compiler, SHALL have the driver of all its cases, written by the build,
compiled by the current compiler and by the previous commit's compiler,
once for each value of runtime error checking among its cells without
original material. The executables and compile results SHALL be the same
unless the difference is named in `PREV_INTENDED`.

#### Scenario: Unchanged rework
- **WHEN** a commit changes no executable and the comparison of a test of the model runs
- **THEN** it passes

#### Scenario: Unintended change
- **WHEN** the executable of a test's driver with runtime error checking differs from the one the previous commit's compiler writes and no entry names it
- **THEN** the comparison fails naming `ID-runerrs`

### Requirement: The entries of a test of the model in PREV_INTENDED
An intended difference of a test of the model SHALL be named `TEST:NAME`,
`TEST` being the comparison's ctest name `PREFIX_ID_prev` and `NAME` `ID`
without and `ID-runerrs` with runtime error checking. An entry SHALL fail
as stale when its compile does not differ or the test has no such compile.

#### Scenario: Intended change
- **WHEN** a difference of the compile with runtime error checking is named as `PREFIX_ID_prev:ID-runerrs`
- **THEN** the comparison passes and reports the difference as intended

#### Scenario: A stale entry
- **WHEN** an entry names `PREFIX_ID_prev:ID-runerrs` and the test has no cell with runtime error checking
- **THEN** the comparison fails naming the entry as stale
