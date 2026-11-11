# Spec Delta

## ADDED Requirements

### Requirement: TED is measured in the coverage configuration
The coverage configuration SHALL build TED so that a TED session records which lines and branches of TED's sources it executed and writes this data to the coverage directory on `TEST:` when TED quits. If the instrumentation does not fit TED's base-relative data, the coverage configuration MAY build TED without base-relative addressing; the normal configuration's TED and its checks SHALL NOT change.

#### Scenario: A TED test leaves coverage data
- **WHEN** a `ted_ab` test or a TED bug-orig test runs in the coverage configuration
- **THEN** the test's coverage directory holds TED's coverage data, and its tracefile names TED functions that the test's script uses

#### Scenario: Normal TED unchanged
- **WHEN** the normal configuration is built
- **THEN** `ted` is byte-identical to the build without this change

### Requirement: The Blitz libraries are measured in the coverage configuration
The coverage configuration SHALL record which code of each Blitz library a test executes, both when the library tests load a library and when a compiled program contains it. The coverage data of a compiled program's libraries SHALL be written when the program ends, by a coverage library that the coverage configuration's compiler links into every program. Libraries whose code conflicts with the instrumentation MAY be left out and SHALL be listed in `tests/README.md`. The library files of the normal configuration SHALL NOT change.

#### Scenario: A library test
- **WHEN** a library test of `libs_tests` runs in the coverage configuration
- **THEN** its tracefile names the functions of the loaded library that the test called

#### Scenario: A compiled program
- **WHEN** a corpus program that uses a library runs in the coverage configuration
- **THEN** its tracefile names the library functions the program executed

### Requirement: Several runs of one test are merged
When one test runs TED, a compiled program or a library test several times, the coverage data of all runs SHALL be merged on the host before the tracefile of the test is written.

#### Scenario: Two runs in one test
- **WHEN** a test runs two compiled programs that use the same library
- **THEN** the test's tracefile counts the library's lines of both runs
