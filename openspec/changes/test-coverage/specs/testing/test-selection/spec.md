# Spec Delta

## Purpose

Chooses the tests a change can affect from the coverage of a full run, so
that work in progress can be checked with a part of the tests; the full
runs stay the acceptance of a program change.

## ADDED Requirements

### Requirement: Coverage of each test as lcov
A full run of the coverage configuration SHALL leave the coverage of every test as an lcov tracefile in the build directory (one file per test, named after the test; `TN:` holds the test's name in the form lcov accepts, every character other than `A-Z`, `a-z`, `0-9` and `_` replaced by `_`), plus a merged tracefile of all tests and a record of the commit the run was made from. Each function a test executed SHALL appear with its source file, its first and last line where known (`FN:first,last,name`) and its hit count (`FNDA`); line and branch counts (`DA`, `BRDA`) SHALL be included where the source of the data gives them. The files SHALL NOT be checked in.

#### Scenario: A function and its tests
- **WHEN** the tracefiles of a full coverage run are read for a function of the TED core that only one host test executes
- **THEN** exactly that test's tracefile lists the function with a hit count above 0

#### Scenario: Compiler functions
- **WHEN** the tracefile of a test that compiled a program with the coverage variant of `blitz2-ab3` is read
- **THEN** it lists the executed functions of `programs/blitz2/src` with their line ranges and no line counts: the only line record of such a function is one `DA` on its first line holding the function's count (its entry, so that lcov's reports show the file), none for its other lines

#### Scenario: Readable by lcov tools
- **WHEN** the merged tracefile is given to `genhtml`
- **THEN** it produces a report without errors

#### Scenario: Origin of the data
- **WHEN** the coverage data is read
- **THEN** the commit of the sources it was made from is known

### Requirement: Tests for a change
For a set of changes against a base commit the selection SHALL read the tracefiles of the last coverage run and name every test that executed a changed function, and SHALL write the names as a list `ctest --tests-from-file` reads.

#### Scenario: Change of one function
- **WHEN** only the body of a function of `programs/blitz2/src/eval.c` changes
- **THEN** the list holds the tests whose coverage contains that function and no test that does not

#### Scenario: Running the selection
- **WHEN** ctest runs with the written list
- **THEN** it runs exactly the selected tests

### Requirement: Changes without function coverage select by rule
Changes the map cannot attribute to functions SHALL select by rule and never fewer tests than could be affected: a changed header SHALL select the tests of every object that includes it; a changed data, test or library source file SHALL select the tests that use it; a change of a host tool (`tools/` and the runners of the test suites, which are not measured) SHALL select the tests that run it: the emulator sessions and what the emulator tests run on the host select every emulator test and the host tests, a tool only the host tests use selects the host tests, and a tool the build runs selects all tests; a change of the build (CMake files, linker scripts, presets), of the toolchain or of a file no rule knows SHALL select all tests. New tests and tests absent from the map SHALL always be selected.

#### Scenario: Header change
- **WHEN** a header included by `programs/blitz2/src/eval.c` and `stmts.c` changes
- **THEN** every test that executed any function of those two objects is selected

#### Scenario: Build change
- **WHEN** a `CMakeLists.txt` changes
- **THEN** all tests are selected

#### Scenario: Change of the emulator sessions
- **WHEN** a source of `tools/lib/amiga_emulation` changes
- **THEN** every emulator test and the host tests are selected

#### Scenario: Change of a tool only the host tests use
- **WHEN** a source of `tools/lib/covmap` changes
- **THEN** the host tests are selected (and the tests absent from the map), no other test

#### Scenario: New test
- **WHEN** a test added after the map was made exists
- **THEN** it is selected

### Requirement: A stale map is reported
The selection SHALL report when the map was made from a commit that is not an ancestor of the base of the changes, and SHALL then select all tests.

#### Scenario: Map from another branch
- **WHEN** the map's commit is not an ancestor of the base commit
- **THEN** the selection says so and selects all tests
