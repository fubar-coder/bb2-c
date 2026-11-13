# testing/expected-results Specification

## Purpose
Makes the expected data of the tests belong to this repository, so that a
test says what is right and a deliberate difference from the original is
visible with its issue number instead of hidden by editing the test.

## Requirements

### Requirement: Corpus expected outputs are committed
Every corpus program SHALL have its expected output in `tests/corpus/expect/PASS/NAME.out` (and, for serial tests, the expected serial bytes in `NAME.serial`; one directory per pass, because a pass with runtime error checking on gives different outputs), written by `corpus-expect-record` from the original's run and reviewed. Each file is one case of the shared test model (`testing/test-model`) with the key `PASS|NAME|OBS|1`. The runner SHALL compare the output of every configuration with the file that applies to it and SHALL NOT read any other table of intended differences.

#### Scenario: Output differs
- **WHEN** a corpus program's output differs from its expect file in any configuration
- **THEN** the test fails and names the program, the configuration and the first differing line

#### Scenario: A fix changes an output
- **WHEN** a fix changes the correct output of a corpus program
- **THEN** the commit turns `NAME.out` into `NAME.orig.out` (the original's output) and `NAME.fixed-N.out` (the port's output) and names the issue; the program source is not edited to avoid the difference

### Requirement: Override rows keep the difference from the original visible
Where expected data is derived from the original (the CD result table `tests/data/blitz2-cd.tsv`, the TED oracle hashes `tests/data/ted-oracle.sha256`), a row holding the port's value for a fixed bug SHALL name the issue (`#NNN`) and the original's value, and the generator SHALL keep such rows when it regenerates the file.

#### Scenario: Regeneration
- **WHEN** the generator of the table or the hashes runs again
- **THEN** every row with an issue number keeps the port's value and the original's value, and no row loses its note

#### Scenario: A fix changes a CD source's result
- **WHEN** a compiler fix changes the executable of a CD source
- **THEN** the table update target rewrites that row with the new value, the issue and the original's hash, and the commit contains the change

### Requirement: Intended-difference lists
`refcheck data`, `rwe_declarations_NAME` and the `refcheck asm` tests SHALL accept an explicit list of intended differences with issue numbers, and each listed entry SHALL occur (a stale entry fails), as `blitzlib compare INTENDED` does. `ted_ab` scripts SHALL take a directive `; @fixed #NNN PAIR...` naming the combo pairs that must differ.

#### Scenario: Stale entry
- **WHEN** a list names a difference that no longer occurs
- **THEN** the test fails and names the entry

#### Scenario: Unlisted difference
- **WHEN** a difference occurs that no list names
- **THEN** the test fails

### Requirement: A fixed library check is a pair of cases
A fixed library check SHALL follow the shared test model (`testing/test-model`): it SHALL be two cases of the library test, `KEY [orig]` with the original's value (run only by `libs_emulator_originals`, preset `originals`) and `KEY [fixed #N]` with the port's value after the fix (run only by `libs_emulator`, preset `all`, no CD needed), both in the committed expected data of the program (`libs/tests/expected/NAME.txt`). The fixed case SHALL be asserted, not skipped. Scenarios SHALL NOT be edited to avoid a difference. No exception table SHALL exist; the earlier `expect_fixed` is replaced by this pair.

#### Scenario: A fixed check
- **WHEN** a library fix changes a result
- **THEN** the expected data has `KEY [orig]` with the original's value and `KEY [fixed #N]` with the port's value, `libs_emulator` requires the second, `libs_emulator_originals` the first, and the other cases of the program compare unchanged

#### Scenario: Test without the CD
- **WHEN** `libs_emulator` runs in `all` without the CD
- **THEN** the port's output of every case equals the expected data case by case, `[fixed #N]` cases included and `[orig]` cases not run

#### Scenario: Scenario changed, expected data not renewed
- **WHEN** a test program is changed and its expected data is not recorded again with `libs-capture`
- **THEN** `libs_emulator` and `libs_emulator_originals` fail on the first differing case

#### Scenario: The port regresses a fixed check
- **WHEN** the port gives the original's value for a `[fixed #N]` case
- **THEN** `libs_emulator` fails on that case

### Requirement: A fixed corpus program is a pair of cases
A corpus program whose original and port differ by a fix SHALL have, for each differing observation (`out`, `serial`, `exe`), a file `NAME.orig.EXT` (the original's value) and a file `NAME.fixed-N.EXT` (the port's value, N the issue) instead of the unqualified file; the `exe` observation SHALL be the SHA-256 of the executable as text. A program with both an unqualified file and a qualified one for the same observation, or with two `fixed` files for one observation, SHALL be reported as malformed with the file name. `tests/corpus/notes.tsv` SHALL NOT exist. The configurations SHALL be judged as follows: `clib` and `prev` against the unqualified and `[fixed #N]` file; `ref` against the unqualified and `[orig]` file; `port` and `orig-clib` against the unqualified file and either qualified file, the report saying which matched; the `exe` cases only in `NAME_originals`, as the port's executable is made with the CD's libraries; a `ref` and `port` executable of a program without `exe` files SHALL be equal.

#### Scenario: The port regresses a fix
- **WHEN** `clib` gives the original's output for a program with a `[fixed #N]` file
- **THEN** `NAME` fails on that case, naming the file and the first differing line

#### Scenario: The original changed
- **WHEN** `ref` gives output that differs from the `[orig]` file
- **THEN** `NAME_originals` fails on that case

#### Scenario: Code-only fix
- **WHEN** a fix changes the port's executable but not the output
- **THEN** the program has `NAME.orig.exe` and `NAME.fixed-N.exe` and no output pair, and `NAME_originals` compares the hashes of `ref` and `port` with them

#### Scenario: Unlisted executable difference
- **WHEN** the `port` executable differs from `ref`'s for a program without `exe` files
- **THEN** `NAME_originals` fails naming the program

### Requirement: Recording and updating the corpus expected data
The build target `corpus-expect-record` (not part of ALL, needs the CD and Amiberry, filterable by `PASS` or `PASS:NAME` in `CORPUS_RECORD_FILTER`) SHALL run the `ref` configuration twice with a different memory layout and SHALL write the unqualified, `[orig]` and `exe` files only if both runs agree, otherwise report "test not deterministic" naming the program and the differing lines and write nothing for it; it SHALL keep every `[fixed #N]` file. The build target `corpus-expect-update` SHALL write from the port (configuration `clib`), SHALL create the files of a new program, SHALL rewrite `[fixed #N]` files, and SHALL refuse to change an unqualified file unless the program is named with its issue (`CORPUS_UPDATE_FIX=PASS:NAME#N`), which turns it into the pair. A test SHALL NOT write any file of the source tree.

#### Scenario: Record with agreeing runs
- **WHEN** the two runs of `ref` give the same output
- **THEN** the files are written and every `[fixed #N]` file is unchanged

#### Scenario: Not deterministic
- **WHEN** the two runs differ for a program
- **THEN** the record reports "test not deterministic" with the program and the differing lines and changes no file of it

#### Scenario: Update refuses a silent change
- **WHEN** `corpus-expect-update` runs without `CORPUS_UPDATE_FIX` and the port's output of a program differs from its unqualified file
- **THEN** it names the program and writes nothing for it

#### Scenario: Update with an issue
- **WHEN** it runs with `CORPUS_UPDATE_FIX=corpus_runerr:t124_bitmaperr#6`
- **THEN** `t124_bitmaperr.out` becomes `t124_bitmaperr.orig.out` with the old content and `t124_bitmaperr.fixed-6.out` has the port's output

### Requirement: The checks of the corpus model have tests with broken input
Every check of this capability SHALL have a test with deliberately broken input that needs no emulator where possible: an unqualified and a qualified file of one observation, two `fixed` files, a qualifier that is neither `orig` nor `fixed-N`, a pair of equal values, a `[fixed #N]` file regressed in the port, an `[orig]` file changed, a missing file, an executable difference without `exe` files, a non-deterministic record refused with nothing written, an update without `FIX` refused.

#### Scenario: Broken input
- **WHEN** a pass runs on an expect directory with two `fixed` files for one program
- **THEN** it exits with status 2 and names the files before any session starts
