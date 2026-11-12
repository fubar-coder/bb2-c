# Spec Delta

## MODIFIED Requirements

### Requirement: Changes without function coverage select by rule
Changes the map cannot attribute to functions SHALL select by rule and never fewer tests than could be affected: a changed header SHALL select the tests of every object that includes it; a changed data, test or library source file SHALL select the tests that use it; a change of a host tool (`tools/` and the runners of the test suites, which are not measured) SHALL select the tests that run it: the emulator sessions and what the emulator tests run on the host select every emulator test and the host tests, a tool only the host tests use selects the host tests, and a tool the build runs selects all tests; a change of the build (CMake files, linker scripts, presets), of the toolchain or of a file no rule knows SHALL select all tests. New tests and tests absent from the map SHALL always be selected. The committed expected and recorded data are inputs of the tests that read them: a changed recorded observation of the original (`tests/bug-orig/*/ID.orig`) SHALL select the bug-orig tests (its own among them), a changed corpus expect file (`tests/corpus/expect/`) SHALL select the corpus tests, and a changed expected-data file of the library tests (`libs/tests/expected/`) SHALL select both library emulator tests (`libs_emulator` and `libs_emulator_originals`).

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

#### Scenario: Recorded observation changes
- **WHEN** `tests/bug-orig/libs/ID.orig` changes
- **THEN** the bug-orig test of `ID` is selected

#### Scenario: Corpus expect file changes
- **WHEN** a file of `tests/corpus/expect/corpus/` changes
- **THEN** the corpus tests are selected

#### Scenario: Library expected data changes
- **WHEN** a file of `libs/tests/expected/` changes
- **THEN** `libs_emulator` and `libs_emulator_originals` are selected
