## ADDED Requirements

### Requirement: Corpus data files are generated, the corpus needs no CD
A corpus program that loads a sound, a module or another data file SHALL read it from the corpus volume (`CORPUS:data/NAME`), staged by the pass from a file the build generates from a text description in the source tree (`tests/corpus/data/NAME.spec`, built with `mktestdata`); no corpus pass SHALL mount the CD volume `BB2:`. Every pass of `ctest --preset all` SHALL therefore run every corpus program of its sources without the CD, none left out for a missing volume. The generated data SHALL be byte-identical for every build, and the original and the port SHALL be given the same files. The build SHALL refuse a corpus source that names a `CORPUS:data/NAME` for which no description exists, and a description no source uses.

#### Scenario: all without the CD
- **WHEN** `ctest --preset all` runs on a machine without `archive/cd`
- **THEN** the passes `corpus` and `corpus_runerr` run `t110_audio`, `t111_tracker` and `t112_med` and their outputs are compared with the committed expected files; the report lists no program as left out

#### Scenario: Same data for original and port
- **WHEN** `corpus-expect-record` records `t112_med` from the original and `ctest --preset all` then runs it with the C libraries
- **THEN** both ran on the same staged file, and a difference in the output fails the test with the first differing line

#### Scenario: A data file nobody describes
- **WHEN** a corpus source loads `CORPUS:data/missing.med` and no `tests/corpus/data/missing.med.spec` exists
- **THEN** configuring the build fails and names the source and the file

#### Scenario: A description nobody uses
- **WHEN** `tests/corpus/data/unused.8svx.spec` exists and no corpus source names `CORPUS:data/unused.8svx`
- **THEN** configuring the build fails and names the description

### Requirement: The generated modules give the player programs something to check
The files `t110_audio`, `t111_tracker` and `t112_med` load SHALL contain what the programs' checks need so that each check can fail: the sample data of the modules is not zero (the checksums before and after the program clears it differ), the sounds have a one-shot part and one of them a repeat part, the notes the MED player reports (`GetMedNote`, `GetMedInstr`, `GetMedVolume`) are non-empty on several channels in the phases the program prints (before and after `JumpMed 3`, with `SetMedVolume` and `SetMedMask`), and the song is long enough for `JumpMed 3` and for a block change within the first 400 pulses. The expected outputs SHALL show it (reviewed when recorded).

#### Scenario: MED notes after a jump
- **WHEN** `t112_med` jumps to song position 3 and plays 60 pulses
- **THEN** the recorded rows of that phase contain notes (not only `--------`)

#### Scenario: Samples are not silent
- **WHEN** `t111_tracker` prints the checksums before and after `silence`
- **THEN** the two values in the committed output differ
