# Spec Delta

## ADDED Requirements

### Requirement: Block observations are hashed by the judge
A program in block mode SHALL write its observations raw to `NAME.bin` beside `NAME.txt`, and the judge SHALL turn them into the same `BLOCK:N` cases at the same place in the output that the program would have written, so that the expected data do not depend on where the hash is computed. A raw file that is truncated, holds an unknown record or a bad length, or does not fit the block places of `NAME.txt` SHALL be a malformed output.

#### Scenario: Same cases as before
- **WHEN** `libs_emulator` runs `systest_med` with block observations written raw
- **THEN** the judge's `BLOCK:N` cases equal the committed expected data byte for byte

#### Scenario: Broken raw file
- **WHEN** `NAME.bin` ends inside a record, holds an unknown record type or a length beyond its end
- **THEN** the compare fails with a message naming the file and the offset

#### Scenario: Raw file missing
- **WHEN** `NAME.txt` has block places but `NAME.bin` is absent
- **THEN** the compare fails naming the missing file

### Requirement: Raw block files are removed after a pass
The raw block files SHALL be deleted when the test or the record that made them passed, and SHALL be kept when it failed, for diagnosis; a run SHALL be able to keep them on request.

#### Scenario: A passed test
- **WHEN** `libs_emulator` passes
- **THEN** no `NAME.bin` is left in its work directory

#### Scenario: A failed test
- **WHEN** `libs_emulator` fails
- **THEN** the `NAME.bin` files stay beside the run's `NAME.txt`

## MODIFIED Requirements

### Requirement: Cases are readable lines with a unique key
The output of a test program SHALL be one case per line, `KEY<TAB>VALUE`, under a header naming the program. The key SHALL be the routine, the check name, the vector (the input of the case) and an occurrence number (counted per routine, check and vector, from 1), joined by `|`; the occurrence number makes the key unique where the same check with the same vector occurs repeatedly. A TAB, a `|` and a `\` in a key part SHALL be escaped as `\t`, `\|` and `\\`, and a non-printable byte as `\xHH`. Values that depend on addresses SHALL be written relative to a stated base (the test program, a library base, a buffer), not as absolute addresses. A byte buffer of up to 64 bytes SHALL be written as hex in the line; a larger one as its length and an FNV hash, and a run with `LIBS_CAP_DUMP` SHALL write the bytes of a named case so that the differing byte can be found; for an observation inside a block the dump SHALL be made on the host from the run's raw block file. The expected data of a program SHALL be one text file, `libs/tests/expected/NAME.txt`, in the same format, in which a key may carry a qualifier (`[orig]`, `[fixed #N]`).

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

#### Scenario: The dump of a block observation
- **WHEN** a run of a block program has `LIBS_CAP_DUMP` naming a block or the key of a bytes observation in a block
- **THEN** the dump file holds the block's observation lines or the bytes, made on the host from the raw block file

### Requirement: Block hashes only for the very large programs and libtest
Only `systest_med`, `rwe_ridisasmlib` and the two `libtest` programs (`ldivlib`, `lmullib`) SHALL write a block of observations as one case holding the block's number, its observation count and a hash (key `BLOCK:N`). Every other program SHALL write its values as cases. Where a block case differs, the compare SHALL name the block and list the block's observations from the run's raw block file; given a second run with its raw block file, it SHALL name the first observation in which the two runs differ. The limits are stated: a fix inside a block program can only be a pair of block cases with hashes nobody can review, two fixes in one block share the pair and one issue number, and a fix that changes the number of observations shifts the block numbers after it.

#### Scenario: A block differs
- **WHEN** a run's hash of block 17 of `rwe_ridisasmlib` differs from the expected data's
- **THEN** the compare names the program and `BLOCK:17` with its observation range and lists the block's observations

#### Scenario: A block differs from another run
- **WHEN** the C build's run and the original's run, both with raw block files, differ in block 17
- **THEN** the compare of the one with the other run given names the first differing observation of block 17 with both values

### Requirement: Record with the original
The build target `libs-capture` SHALL run every test program with `LIBSET=orig` twice with different memory layouts and SHALL write the expected data `libs/tests/expected/NAME.txt` from the original only if both outputs are identical; it SHALL need the CD and SHALL NOT be a ctest test. Otherwise it SHALL report the differing cases as "test not deterministic", name the program (and, for a differing block, its first differing observation), and write nothing. It SHALL refuse a result that holds a `lost` case or a `lost` observation inside a block (the stated set is too strict for the original). When it rewrites a file it SHALL write the original's values as unqualified or `[orig]` cases and keep every `[fixed #N]` case as it is. A part whose result cannot be reproduced SHALL be written as one case `not captured: REASON` in place of its observations and SHALL NOT be compared. A run limited to some programs SHALL leave the files of the others untouched. The proof of determinism is limited to the memory layout: date, time and emulator timing are not varied.

#### Scenario: Deterministic program
- **WHEN** the two runs of a program give identical output
- **THEN** the file is written to `libs/tests/expected/` with the `[fixed #N]` cases kept

#### Scenario: Non-deterministic program
- **WHEN** the two runs differ in a case
- **THEN** `libs-capture` reports "test not deterministic" with the differing cases and leaves the committed file as it was

#### Scenario: Non-deterministic block
- **WHEN** the two runs differ in an observation inside a block
- **THEN** the report names the block and that observation with the values of both runs

#### Scenario: A lost register inside a block
- **WHEN** an observation inside a block of the original's run is `lost`
- **THEN** `libs-capture` refuses the program naming the observation and writes nothing

#### Scenario: An unreproducible part
- **WHEN** a program has a part whose result varies for a reason outside the library
- **THEN** its output has a `not captured: REASON` case there and no observation cases of that part, and the part is not compared

#### Scenario: A filtered run
- **WHEN** `libs-capture` runs for one program
- **THEN** the expected data of the other programs is neither read as missing nor changed
