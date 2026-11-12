# Spec Delta

## Purpose
Defines how the library tests observe one library at a time, write what
they see as readable text, and compare the C build with the committed
observations of the original, so that no comparison can silently compare
the C build with itself.

## ADDED Requirements

### Requirement: A test program runs one library set
Every library test program SHALL load exactly one set of library files, selected by the run (`NAME.obj` of the C build or `NAME.orig` of the original), and SHALL write its observations as output. A program SHALL NOT load the other set, and SHALL NOT compare values of two sets itself.

#### Scenario: Run with the C build
- **WHEN** a test program runs with the C build's files
- **THEN** it loads only those files and writes its observations to its output file

#### Scenario: Run with the original
- **WHEN** the same program runs with the original's files
- **THEN** it executes the same scenario and writes output in the same format

### Requirement: Observations are readable lines
The output of a test program SHALL have one line per check: the check name, the input or vector, and the observed value, in a fixed text format with a header naming the program. Values that depend on addresses SHALL be written relative to a stated base (the test program, a library base, a buffer), not as absolute addresses. The committed capture of a program SHALL be one text file, `libs/tests/captures/NAME.txt`.

#### Scenario: A value differs
- **WHEN** the C build's output has a line whose value differs from the capture's
- **THEN** the compare names the line number, the check and input, the captured value and the C build's value

#### Scenario: Lines are missing or added
- **WHEN** the C build's output has fewer or more lines than the capture
- **THEN** the compare fails and names the first line that has no counterpart

#### Scenario: Address independence
- **WHEN** the libraries load at other addresses than in the run that produced the capture
- **THEN** the output is unchanged

### Requirement: Block hashes only for very large programs
A test program MAY write a block of observations as one line holding the block's number, its observation count and a hash, if the program makes more than the budget stated in the design (one million observations). Every other program SHALL write its values. Where a block line differs, the compare SHALL name the block.

#### Scenario: A block differs
- **WHEN** the C build's hash of block 17 of `ridisasmlib` differs from the capture's
- **THEN** the compare names the program and block 17 with its observation range

### Requirement: Kept registers are checked by the test program
For every routine call a test program SHALL check the registers that the Blitz convention requires to be kept (everything except what the routine may change, `docs/libs/deviations.md` "Register preservation"; the stricter set the port agrees on, the registers the original keeps, where a test states it) and SHALL write one line `kept` or `lost REGISTERS`. The comparison of C build and capture SHALL be line equality, with no subset rule.

#### Scenario: Original and C build agree
- **WHEN** both keep all required registers
- **THEN** both outputs have the same `kept` line

#### Scenario: The C build loses a required register
- **WHEN** the C build changes a register that is required to be kept
- **THEN** its line says `lost` with the register, the capture says `kept`, and the compare fails on that line

### Requirement: Runs are deterministic or say why not
A test program SHALL give the same output in every run of the same library set. Pseudo-random input SHALL use a fixed seed, and hardware state a test depends on SHALL be put into a defined state by the program. A part whose result cannot be reproduced SHALL be written as one line `not captured: REASON` in place of its observations and SHALL NOT be compared.

#### Scenario: Two capture runs
- **WHEN** the capture run with the original is done twice
- **THEN** the two outputs are byte-identical

#### Scenario: An unreproducible part
- **WHEN** a program has a part whose result varies from run to run for a reason outside the library
- **THEN** its output has a `not captured: REASON` line there and no observation lines of that part

### Requirement: Where the runs belong
The compare run (C build against the committed captures) SHALL run in the presets `all` and `emulator` and SHALL NOT need the CD. The capture run with the original and the check that a fresh capture equals the committed file SHALL run in the preset `originals` (label `originals`) and need the CD.

#### Scenario: Without the CD
- **WHEN** `ctest --preset all` runs on a machine without `archive/cd`
- **THEN** the compare run executes and passes, and the capture run is skipped

#### Scenario: Original or test changed
- **WHEN** the original's files or a scenario changed and the committed capture was not renewed
- **THEN** `ctest --preset originals` fails and names the first differing line

### Requirement: The checks have tests with broken input
Every check of this capability SHALL have a test with deliberately broken input that fails with the check's message: a changed value, a missing or an added line, a changed block hash, a lost required register, a stale or wrong exceptions row, a capture that differs from the committed one.

#### Scenario: Broken capture
- **WHEN** a capture with one value changed is compared
- **THEN** the compare fails naming that line
