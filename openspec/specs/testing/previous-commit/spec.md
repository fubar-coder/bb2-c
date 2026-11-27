# testing/previous-commit Specification

## Purpose
Provides the build of the previous commit that the acceptance rule of
2026-11-01 compares with, so that "byte-identical unless intended" is a
test and not a manual step. It covers the results of the build and the tests
of the BASIC test model (`testing/basic-test-runner`).

## Requirements

### Requirement: Build of the previous commit on request
A CMake option (`BLITZ2_PREV`, default `HEAD~1`, or a path to a build directory) SHALL build the Amiga side of the given commit in a build subdirectory (`prev/`) from `git archive`: blitz2, blitz2-ab3, ted, acidlibs, deflibs, the variants and the userlibs. The default configure SHALL NOT build it.

#### Scenario: Not requested
- **WHEN** the project is configured without the option
- **THEN** no second build is made and no ctest depends on it

#### Scenario: Requested
- **WHEN** the project is configured with the option and built
- **THEN** the results of the commit are in `prev/blitz2/` and the current checkout is untouched

### Requirement: Byte comparison of results
A host test `prev_results` SHALL compare every library file, `acidlibs` and `deflibs` byte by byte with the `prev` build and fail for a difference that is not named in an intended list or by `-DPREV_INTENDED=ON`. The corpus SHALL compare the executables of the current compilers with those the `prev` compiler writes, and `ted_ab` SHALL have the combos `P5`/`Pc` for the previous TED.

#### Scenario: Unchanged rework
- **WHEN** a commit changes no result and `prev_results` runs
- **THEN** it passes

#### Scenario: Unintended change
- **WHEN** a library file differs from the `prev` build and is not listed as intended
- **THEN** the test fails and names the file

#### Scenario: Intended change
- **WHEN** a fix changes a library file and the file is listed as intended
- **THEN** the test passes and the commit names the file

### Requirement: Absence of the prev build
Tests that need the `prev` build SHALL report skipped (return value 77) when it is not built, not pass.

#### Scenario: No prev build
- **WHEN** `ctest --preset all` runs without a `prev` build
- **THEN** the dependent tests are reported as skipped

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
