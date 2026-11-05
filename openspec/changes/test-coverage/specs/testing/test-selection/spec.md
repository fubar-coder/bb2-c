# Spec Delta

## Purpose

Chooses the tests a change can affect from the coverage of a full run, so
that work in progress can be checked with a part of the tests; the full
runs stay the acceptance of a program change.

## ADDED Requirements

### Requirement: Map from functions to tests
A full run of the coverage configuration SHALL produce a map that names, for every function executed by any test, the tests that executed it, together with the commit it was made from. The map SHALL be generated in the build directory and SHALL NOT be checked in.

#### Scenario: A function and its tests
- **WHEN** the map of a full coverage run is read for a function of the TED core that only one host test executes
- **THEN** it names exactly that test

#### Scenario: Origin of the map
- **WHEN** the map is read
- **THEN** it states the commit of the sources it was made from

### Requirement: Tests for a change
For a set of changes against a base commit the selection SHALL name every test that executed a changed function according to the map, and SHALL write the names as a list `ctest --tests-from-file` reads.

#### Scenario: Change of one function
- **WHEN** only the body of a function of `programs/blitz2/src/eval.c` changes
- **THEN** the list holds the tests whose coverage contains that function and no test that does not

#### Scenario: Running the selection
- **WHEN** ctest runs with the written list
- **THEN** it runs exactly the selected tests

### Requirement: Changes without function coverage select by rule
Changes the map cannot attribute to functions SHALL select by rule and never fewer tests than could be affected: a changed header SHALL select the tests of every object that includes it; a changed data, test or library source file SHALL select the tests that use it; a change of the build (CMake files, linker scripts, presets), of the toolchain or of a file no rule knows SHALL select all tests. New tests and tests absent from the map SHALL always be selected.

#### Scenario: Header change
- **WHEN** a header included by `programs/blitz2/src/eval.c` and `stmts.c` changes
- **THEN** every test that executed any function of those two objects is selected

#### Scenario: Build change
- **WHEN** a `CMakeLists.txt` changes
- **THEN** all tests are selected

#### Scenario: New test
- **WHEN** a test added after the map was made exists
- **THEN** it is selected

### Requirement: A stale map is reported
The selection SHALL report when the map was made from a commit that is not an ancestor of the base of the changes, and SHALL then select all tests.

#### Scenario: Map from another branch
- **WHEN** the map's commit is not an ancestor of the base commit
- **THEN** the selection says so and selects all tests
