# Spec Delta

## Purpose
Defines how the library tests implement the shared test model
(`testing/test-model`): a program observes one library set at a time,
writes what it sees as readable cases, and a judge compares the run with
the committed expected data, so that no comparison can silently compare the
C build with itself. Recording the expected data and testing are separate
operations.

## ADDED Requirements

### Requirement: A test program runs one library set
Every library test program SHALL load exactly one set of library files, selected by the run through the variable `LIBSET` (`obj`: `NAME.obj` of the C build; `orig`: `NAME.orig` of the original), and SHALL write its observations as output. A program SHALL NOT load the other set, and SHALL NOT compare values of two sets itself.

#### Scenario: Run with the C build
- **WHEN** a test program runs with `LIBSET=obj`
- **THEN** it loads only the C build's files and writes its observations to its output file

#### Scenario: Run with the original
- **WHEN** the same program runs with `LIBSET=orig`
- **THEN** it executes the same scenario and writes output in the same format

### Requirement: Cases are readable lines with a unique key
The output of a test program SHALL be one case per line, `KEY<TAB>VALUE`, under a header naming the program. The key SHALL be the routine, the check name, the vector (the input of the case) and an occurrence number (counted per routine, check and vector, from 1), joined by `|`; the occurrence number makes the key unique where the same check with the same vector occurs repeatedly. A TAB, a `|` and a `\` in a key part SHALL be escaped as `\t`, `\|` and `\\`, and a non-printable byte as `\xHH`. Values that depend on addresses SHALL be written relative to a stated base (the test program, a library base, a buffer), not as absolute addresses. A byte buffer of up to 64 bytes SHALL be written as hex in the line; a larger one as its length and an FNV hash, and a run with `LIBS_CAP_DUMP` SHALL write the bytes of a named case so that the differing byte can be found. The expected data of a program SHALL be one text file, `libs/tests/expected/NAME.txt`, in the same format, in which a key may carry a qualifier (`[orig]`, `[fixed #N]`).

#### Scenario: A value differs
- **WHEN** a run's output has a case whose value differs from the expected data
- **THEN** the compare names the key, the expected value and the run's value

#### Scenario: A case is missing or added
- **WHEN** a run's output lacks a key of the expected data, or has one it does not have
- **THEN** the compare fails and names the key

#### Scenario: The same check twice
- **WHEN** the same check with the same vector occurs twice in a routine
- **THEN** the two cases have the occurrence numbers 1 and 2 and are judged separately

#### Scenario: Address independence
- **WHEN** the libraries load at other addresses than in the run that produced the expected data
- **THEN** the output is unchanged

#### Scenario: A long buffer differs
- **WHEN** a buffer of more than 64 bytes differs
- **THEN** the compare names the key and both hashes, and the run with `LIBS_CAP_DUMP` for that case gives the bytes

### Requirement: Qualified cases select the library set
The judge SHALL apply to a run with `LIBSET=orig` the cases without a qualifier and the `[orig]` cases, and to a run with `LIBSET=obj` the cases without a qualifier and the `[fixed #N]` cases; the other qualified cases SHALL NOT be run against that set. An unqualified key together with a qualified one of the same key, or a qualifier other than `[orig]` and `[fixed #N]`, SHALL be a malformed file reported with its line.

#### Scenario: Port run
- **WHEN** `libs_emulator` judges a run and the expected data has `KEY [orig]` and `KEY [fixed #7]`
- **THEN** the run's `KEY` is compared with the `[fixed #7]` value and the `[orig]` case is ignored

#### Scenario: Original run
- **WHEN** `libs_emulator_originals` judges a run with the same expected data
- **THEN** the run's `KEY` is compared with the `[orig]` value and the `[fixed #7]` case is ignored

#### Scenario: Malformed file
- **WHEN** a file holds `KEY` and `KEY [orig]`
- **THEN** the judge fails naming the line

### Requirement: Block hashes only for the very large programs and libtest
Only `systest_med`, `rwe_ridisasmlib` and the two `libtest` programs (`ldivlib`, `lmullib`) SHALL write a block of observations as one case holding the block's number, its observation count and a hash (key `BLOCK:N`). Every other program SHALL write its values as cases. Where a block case differs, the compare SHALL name the block. The limits are stated: a fix inside a block program can only be a pair of block cases with hashes nobody can review, two fixes in one block share the pair and one issue number, and a fix that changes the number of observations shifts the block numbers after it.

#### Scenario: A block differs
- **WHEN** a run's hash of block 17 of `rwe_ridisasmlib` differs from the expected data's
- **THEN** the compare names the program and `BLOCK:17` with its observation range

### Requirement: Kept registers are checked by the test program
For every routine the test program SHALL state the set of registers that the original keeps in all calls of the scenario (the intersection over the calls, never less than the Blitz convention requires, `docs/libs/deviations.md` "Register preservation"), SHALL check that set after each call and SHALL write one case with the value `kept` or `lost REGISTERS`. The comparison of a run and the expected data SHALL be equality, with no subset rule. A register the original keeps only in some calls is not covered; this is accepted.

#### Scenario: Original and C build agree
- **WHEN** both keep all stated registers
- **THEN** both outputs have the same `kept` case

#### Scenario: The C build loses a stated register
- **WHEN** the C build changes a register that is in the routine's stated set
- **THEN** its case says `lost` with the register, the expected data says `kept`, and the compare fails on that key

### Requirement: Record with the original
The build target `libs-capture` SHALL run every test program with `LIBSET=orig` twice with different memory layouts and SHALL write the expected data `libs/tests/expected/NAME.txt` from the original only if both outputs are identical; it SHALL need the CD and SHALL NOT be a ctest test. Otherwise it SHALL report the differing cases as "test not deterministic", name the program, and write nothing. It SHALL refuse a result that holds a `lost` case (the stated set is too strict for the original). When it rewrites a file it SHALL write the original's values as unqualified or `[orig]` cases and keep every `[fixed #N]` case as it is. A part whose result cannot be reproduced SHALL be written as one case `not captured: REASON` in place of its observations and SHALL NOT be compared. A run limited to some programs SHALL leave the files of the others untouched. The proof of determinism is limited to the memory layout: date, time and emulator timing are not varied.

#### Scenario: Deterministic program
- **WHEN** the two runs of a program give identical output
- **THEN** the file is written to `libs/tests/expected/` with the `[fixed #N]` cases kept

#### Scenario: Non-deterministic program
- **WHEN** the two runs differ in a case
- **THEN** `libs-capture` reports "test not deterministic" with the differing cases and leaves the committed file as it was

#### Scenario: An unreproducible part
- **WHEN** a program has a part whose result varies for a reason outside the library
- **THEN** its output has a `not captured: REASON` case there and no observation cases of that part, and the part is not compared

#### Scenario: A filtered run
- **WHEN** `libs-capture` runs for one program
- **THEN** the expected data of the other programs is neither read as missing nor changed

### Requirement: Update from the port
The build target `libs-update` SHALL run the programs with `LIBSET=obj` and rewrite the value of every `[fixed #N]` case from the port's output; naming a key and an issue (`FIX=KEY#N`) SHALL turn that unqualified case into the pair `KEY [orig]` (the old value) and `KEY [fixed #N]` (the port's value). It SHALL NOT need the CD and SHALL NOT change any other case. A hand-written `[fixed #N]` value SHALL use the same format.

#### Scenario: A fix changes a case
- **WHEN** `libs-update` runs with `FIX=KEY#7` and the port's value of `KEY` differs from the expected one
- **THEN** the file holds `KEY [orig]` with the old value and `KEY [fixed #7]` with the port's value, and nothing else changes

### Requirement: Test with either library set
`libs_emulator` (label `emulator`; presets `all` and `emulator`; no CD) SHALL run every program with `LIBSET=obj` and `libs_emulator_originals` (label `originals`; needs the CD) SHALL run every program with `LIBSET=orig`; both SHALL only compare with the committed expected data, through the same judge, and SHALL NOT write committed files. The run's output of `libs_emulator` SHALL stay in its work directory (`build/<preset>/libs/tests/`) so that it can be diffed against the expected data; there is no separate operation for that.

#### Scenario: Without the CD
- **WHEN** `ctest --preset all` runs on a machine without `archive/cd`
- **THEN** `libs_emulator` runs and passes, and `libs_emulator_originals` is skipped

#### Scenario: Original or test changed
- **WHEN** the original's files or a scenario changed and the expected data was not recorded again
- **THEN** `ctest --preset originals` fails and names the first differing key

#### Scenario: Debugging a failure
- **WHEN** `libs_emulator` fails for a program
- **THEN** the program's full output is in the test's work directory

### Requirement: The judge is a C tool
Comparing a run with the expected data and accepting two recording runs SHALL be the C tool `tools/results` (`libs-compare`, `libs-accept`, `libs-update`; logic in `tools/lib`), with host tests from memory; CMake SHALL only orchestrate the runs. The expected data SHALL be marked `-text diff` in `.gitattributes`.

#### Scenario: A compare of 3 million cases
- **WHEN** the judge compares `systest_med` blocks and the long programs
- **THEN** it reads both files in one pass and its time is not the dominant cost of the test

### Requirement: The checks have tests with broken input
Every check of this capability SHALL have a test with deliberately broken input that fails with the check's message: a changed value, a missing or an added case, a changed block hash, a lost stated register, a malformed or ambiguous expected file, a `[fixed #N]` case regressed in the port, an `[orig]` case changed, a non-deterministic recording refused, a run that differs from the expected data.

#### Scenario: Broken run
- **WHEN** a run with one value changed is compared
- **THEN** the compare fails naming that key

#### Scenario: Non-deterministic recording
- **WHEN** two recording outputs of a program differ in one case
- **THEN** the accept step fails with "test not deterministic", names the case and writes no committed file
