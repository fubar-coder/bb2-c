## MODIFIED Requirements

### Requirement: Override rows keep the difference from the original visible
Where expected data is derived from the original (the CD result table `tests/data/blitz2-cd.tsv`, the TED oracle hashes `tests/data/ted-oracle.sha256`), a row holding the port's value for a fixed bug SHALL name the issue (`#NNN`) and the original's value, and the generator SHALL keep such rows when it regenerates the file. In the CD result table only a differing compile result (an error the original does not give, another error line, a source the port refuses) SHALL need such a note; the size and hash of an `ok` row SHALL be the original's, SHALL be reference data, and a port that compiles the source to another executable SHALL NOT need a note, a row change or an issue.

#### Scenario: Regeneration
- **WHEN** the generator of the table or the hashes runs again
- **THEN** every row with an issue number keeps the port's value and the original's value, and no row loses its note

#### Scenario: A fix changes a CD source's result
- **WHEN** a compiler fix changes the compile result of a CD source (ok or the error line)
- **THEN** the table update target rewrites that row with the new value, the issue and the original's result, and the commit contains the change

#### Scenario: An optimization changes an executable
- **WHEN** the port compiles a CD source to another executable and the result stays `ok`
- **THEN** `corpus_cd` passes and reports the row as an informational executable difference, and the table update leaves the row as it is

### Requirement: A fixed corpus program is a pair of cases
A corpus program whose original and port differ by a fix SHALL have, for each differing observation (`out`, `serial`), a file `NAME.orig.EXT` (the original's value) and a file `NAME.fixed-N.EXT` (the port's value, N the issue) instead of the unqualified file. A program with both an unqualified file and a qualified one for the same observation, with two `fixed` files for one observation, or with a file of another observation (`NAME.exe`, `NAME.orig.exe`) SHALL be reported as malformed with the file name. `tests/corpus/notes.tsv` SHALL NOT exist. The configurations SHALL be judged as follows: `clib` and `prev` against the unqualified and `[fixed #N]` file; `ref` against the unqualified and `[orig]` file; `port` and `orig-clib` against the unqualified file and either qualified file, the report saying which matched. The executable of `port` SHALL be compared with `ref`'s for information only (a difference is reported, the test does not fail on it); a `port` that makes no executable where `ref` does SHALL fail.

#### Scenario: The port regresses a fix
- **WHEN** `clib` gives the original's output for a program with a `[fixed #N]` file
- **THEN** `NAME` fails on that case, naming the file and the first differing line

#### Scenario: The original changed
- **WHEN** `ref` gives output that differs from the `[orig]` file
- **THEN** `NAME_originals` fails on that case

#### Scenario: Code-only fix
- **WHEN** a fix or an optimization changes the port's executable but not the output
- **THEN** the program has no expect file for it and `NAME_originals` passes, reporting `exe differs (informational)`

#### Scenario: Unlisted executable difference
- **WHEN** the `port` executable differs from `ref`'s for a program and the output is as expected
- **THEN** `NAME_originals` passes and names the difference as informational; it fails only if the output differs or `port` makes no executable

#### Scenario: A stale executable file
- **WHEN** an expect directory holds `NAME.orig.exe` or `NAME.fixed-N.exe`
- **THEN** the pass exits with status 2 naming the file before any session starts

### Requirement: Recording and updating the corpus expected data
The build target `corpus-expect-record` (not part of ALL, needs the CD and Amiberry, filterable by `PASS` or `PASS:NAME` in `CORPUS_RECORD_FILTER`) SHALL run the `ref` configuration twice with a different memory layout and SHALL write the unqualified and `[orig]` files only if both runs agree, otherwise report "test not deterministic" naming the program and the differing lines and write nothing for it; it SHALL keep every `[fixed #N]` file. The build target `corpus-expect-update` SHALL write from the port (configuration `clib`), SHALL create the files of a new program, SHALL rewrite `[fixed #N]` files, and SHALL refuse to change an unqualified file unless the program is named with its issue (`CORPUS_UPDATE_FIX=PASS:NAME#N`), which turns it into the pair. Neither target SHALL write an executable's hash. A test SHALL NOT write any file of the source tree.

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
Every check of this capability SHALL have a test with deliberately broken input that needs no emulator where possible: an unqualified and a qualified file of one observation, two `fixed` files, a qualifier that is neither `orig` nor `fixed-N`, a pair of equal values, a `[fixed #N]` file regressed in the port, an `[orig]` file changed, a missing file, an executable file (`NAME.exe`, a pair of them), an executable difference of `port` that only is reported, a `port` without an executable, a non-deterministic record refused with nothing written, an update without `FIX` refused.

#### Scenario: Broken input
- **WHEN** a pass runs on an expect directory with two `fixed` files for one program
- **THEN** it exits with status 2 and names the files before any session starts
