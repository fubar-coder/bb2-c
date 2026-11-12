# Spec Delta

## Purpose
Defines how the library tests observe one library at a time, write what
they see as readable text, and compare a run with the committed
observations of the original, so that no comparison can silently compare
the C build with itself. Capture and test are separate operations, each
possible with either library set.

## ADDED Requirements

### Requirement: A test program runs one library set
Every library test program SHALL load exactly one set of library files, selected by the run through the variable `LIBSET` (`obj`: `NAME.obj` of the C build; `orig`: `NAME.orig` of the original), and SHALL write its observations as output. A program SHALL NOT load the other set, and SHALL NOT compare values of two sets itself.

#### Scenario: Run with the C build
- **WHEN** a test program runs with `LIBSET=obj`
- **THEN** it loads only the C build's files and writes its observations to its output file

#### Scenario: Run with the original
- **WHEN** the same program runs with `LIBSET=orig`
- **THEN** it executes the same scenario and writes output in the same format

### Requirement: Observations are readable lines
The output of a test program SHALL have one line per check: the check name, the input or vector, and the observed value, in a fixed text format with a header naming the program. Values that depend on addresses SHALL be written relative to a stated base (the test program, a library base, a buffer), not as absolute addresses. A byte buffer of up to 64 bytes SHALL be written as hex in the line; a larger one as its length and an FNV hash, and a run with `LIBS_CAP_DUMP` SHALL write the bytes of a named check so that the differing byte can be found. The committed capture of a program SHALL be one text file, `libs/tests/captures/NAME.txt`.

#### Scenario: A value differs
- **WHEN** a run's output has a line whose value differs from the capture's
- **THEN** the compare names the line number, the check and input, the captured value and the run's value

#### Scenario: Lines are missing or added
- **WHEN** a run's output has fewer or more lines than the capture
- **THEN** the compare fails and names the first line that has no counterpart

#### Scenario: Address independence
- **WHEN** the libraries load at other addresses than in the run that produced the capture
- **THEN** the output is unchanged

#### Scenario: A long buffer differs
- **WHEN** a buffer of more than 64 bytes differs
- **THEN** the compare names the check, the vector and both hashes, and the run with `LIBS_CAP_DUMP` for that check gives the bytes

### Requirement: Block hashes only for two very large programs
Only `systest_med` and `rwe_ridisasmlib` SHALL write a block of observations as one line holding the block's number, its observation count and a hash. Every other program, `libtest` included, SHALL write its values as lines. Where a block line differs, the compare SHALL name the block.

#### Scenario: A block differs
- **WHEN** a run's hash of block 17 of `rwe_ridisasmlib` differs from the capture's
- **THEN** the compare names the program and block 17 with its observation range

### Requirement: Kept registers are checked by the test program
For every routine the test program SHALL state the set of registers that the original keeps in all calls of the scenario (the intersection over the calls, never less than the Blitz convention requires, `docs/libs/deviations.md` "Register preservation"), SHALL check that set after each call and SHALL write one line `kept` or `lost REGISTERS`. The comparison of a run and the capture SHALL be line equality, with no subset rule. A register the original keeps only in some calls is not covered; this is accepted.

#### Scenario: Original and C build agree
- **WHEN** both keep all stated registers
- **THEN** both outputs have the same `kept` line

#### Scenario: The C build loses a stated register
- **WHEN** the C build changes a register that is in the routine's stated set
- **THEN** its line says `lost` with the register, the capture says `kept`, and the compare fails on that line

### Requirement: Capture with the original
The build target `libs_capture` SHALL run every test program with `LIBSET=orig` and write the committed reference `libs/tests/captures/NAME.txt`; it SHALL need the CD and SHALL NOT be a ctest test. It SHALL run each program twice with different memory layouts and SHALL accept the result only if both outputs are identical. Otherwise it SHALL report the differing lines as "test not deterministic", name the program, and write nothing. It SHALL refuse a capture that holds a `lost` line (the stated set is too strict for the original). A part whose result cannot be reproduced SHALL be written as one line `not captured: REASON` in place of its observations and SHALL NOT be compared.

#### Scenario: Deterministic program
- **WHEN** the two runs of a program give identical output
- **THEN** the file is written to `libs/tests/captures/`

#### Scenario: Non-deterministic program
- **WHEN** the two runs differ in a line
- **THEN** `libs_capture` reports "test not deterministic" with the differing lines and leaves the committed file as it was

#### Scenario: An unreproducible part
- **WHEN** a program has a part whose result varies for a reason outside the library
- **THEN** its output has a `not captured: REASON` line there and no observation lines of that part, on both sides of every comparison

### Requirement: Capture with the C build is a debugging aid
`libs_capture` with `LIBSET=obj` SHALL write the C build's output to the build directory, report its differences from the committed reference, and SHALL NOT write to `libs/tests/captures/`. It SHALL NOT need the CD.

#### Scenario: Debugging a failure
- **WHEN** `libs_test` fails for a program
- **THEN** `libs_capture` with `LIBSET=obj` leaves that program's full output in the build directory for a diff against the reference

### Requirement: Test with either library set
`libs_test` (label `emulator`; presets `all` and `emulator`; no CD) SHALL run every program with `LIBSET=obj` and compare with the committed captures, applying the exceptions table. `libs_test_originals` (label `originals`; needs the CD) SHALL run every program with `LIBSET=orig` and compare with the committed captures with the same comparison code, applying no exceptions; its failure means that the test is not deterministic or that a file of the CD changed.

#### Scenario: Without the CD
- **WHEN** `ctest --preset all` runs on a machine without `archive/cd`
- **THEN** `libs_test` runs and passes, and `libs_test_originals` is skipped

#### Scenario: Original or test changed
- **WHEN** the original's files or a scenario changed and the committed capture was not renewed
- **THEN** `ctest --preset originals` fails and names the first differing line

### Requirement: The checks have tests with broken input
Every check of this capability SHALL have a test with deliberately broken input that fails with the check's message: a changed value, a missing or an added line, a changed block hash, a lost stated register, a stale or wrong exceptions row, a non-deterministic capture refused, a run that differs from the committed capture.

#### Scenario: Broken run
- **WHEN** a run with one value changed is compared
- **THEN** the compare fails naming that line

#### Scenario: Non-deterministic capture
- **WHEN** two capture outputs of a program differ in one line
- **THEN** the accept step fails with "test not deterministic", names the line and writes no committed file
