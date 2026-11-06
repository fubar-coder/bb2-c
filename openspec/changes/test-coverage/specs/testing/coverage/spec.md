# Spec Delta

## Purpose

Records which code each test executes - on the host side and in the
compilers run in the emulator - without changing the normal build or the
results it produces, as the basis for choosing tests and for seeing code
no test reaches.

## ADDED Requirements

### Requirement: Host coverage is an option of the host build
The host build SHALL have an option that builds the program code the host tests compile (the sources of the programs and the shared code, `programs/*/src` and `shared/`) with coverage instrumentation, off by default; the tools (`tools/`), the test sources and the tools of the programs SHALL NOT be instrumented. With the option, every host test SHALL leave its coverage data apart from the other tests', so that the data of one test can be read on its own.

#### Scenario: Coverage of one host test
- **WHEN** the host side is built with the coverage option and the host tests run
- **THEN** each test's coverage data is stored under the test's name, and reading it lists the functions that test executed (for example the TED core functions of a TED host test)

#### Scenario: Tools are not measured
- **WHEN** the host side is built with the coverage option and a host test of a tool runs (for example one of the emulator sessions in `tools/lib/amiga_emulation`)
- **THEN** its coverage data names no function of `tools/` and of the test sources

#### Scenario: Default build
- **WHEN** the host side is built without the option
- **THEN** no host program is instrumented and no coverage data is written

### Requirement: Coverage variants of the compilers
A separate coverage configuration SHALL build `blitz2` and `blitz2-ab3` with every function call recorded. The variants SHALL generate byte-identical executables for the same source and options as the normal compilers, except for bytes that depend on where the compiler's own memory lies (the editor line references behind the runtime error checks' traps, padding the compiler does not write), as the corpus runner already masks them, SHALL run on Kickstart 1.3 like them, and SHALL keep base-relative addressing. The normal configuration's programs, libraries and checks SHALL NOT change.

#### Scenario: Same output
- **WHEN** a source of the corpus is compiled once with `blitz2` and once with its coverage variant
- **THEN** the two executables are byte-identical

#### Scenario: Normal build unchanged
- **WHEN** the normal configuration is built before and after this change
- **THEN** `blitz2`, `blitz2-ab3`, TED and all library files are byte-identical

#### Scenario: Kickstart 1.3
- **WHEN** a test on the A500 machine (Kickstart 1.3) runs the coverage variant of `blitz2`
- **THEN** it compiles as the normal `blitz2` does and leaves its coverage data

### Requirement: Coverage data of a compiler run
At its end a coverage variant SHALL add the functions it executed to a coverage file named after the program in the coverage directory on `TEST:`, merging with what earlier runs of the same test wrote there. Without that directory it SHALL write nothing and behave as the normal compiler; a failure to write SHALL NOT change the compiler's result or return code.

#### Scenario: Several compiles in one test
- **WHEN** a test compiles several sources with the coverage variant in one emulator session
- **THEN** the coverage file holds every function executed in any of the runs

#### Scenario: No coverage directory
- **WHEN** the coverage variant runs where `TEST:` has no coverage directory
- **THEN** it writes no file, shows no requester and gives the same result as the normal compiler

### Requirement: Collecting the data of an emulator test
In the coverage configuration every emulator test that runs a compiler SHALL create the coverage directory before the run and SHALL copy the coverage files back to the test's work directory on the host afterwards, one set per ctest test.

#### Scenario: A coverage test run
- **WHEN** the coverage test preset runs the corpus and the `ab3_cli` tests
- **THEN** each of these tests leaves its compilers' coverage files in its work directory

### Requirement: Functions named by the map files
The coverage of a compiler SHALL be read as functions: each recorded address SHALL be mapped to the function containing it with the symbols of the program's link map.

#### Scenario: Reading a coverage file
- **WHEN** the coverage file of a test that compiled one program is read with the map of `blitz2-ab3`
- **THEN** the result names functions of `programs/blitz2/src` (for example the parser's and the code generator's), each once
