# testing/previous-commit Specification

## Purpose
Provides the build of the previous commit that the acceptance rule of
2026-11-01 compares with, so that "byte-identical unless intended" is a
test and not a manual step.

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
