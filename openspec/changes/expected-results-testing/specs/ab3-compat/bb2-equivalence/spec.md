# Spec Delta

## MODIFIED Requirements

### Requirement: The default blitz2 is unchanged
`blitz2`'s results SHALL stay byte-identical to those of the build of the previous commit (the executables it writes, and `ted`'s, `acidlibs`', `deflibs`' and all library files) unless the commit names an intended change; the AB3 mode SHALL only be part of `blitz2-ab3`. The comparison is made with the `prev` build (`testing/previous-commit`). The `blitz2` program file itself may change with a code rework (rule of 2026-11-01).

#### Scenario: Default build
- **WHEN** `blitz2` is built before and after a code rework and both compile the corpus, the bug-orig tests and the CD's sources
- **THEN** every executable written by the two builds is byte-identical, and so are `ted`'s output files, `acidlibs`, `deflibs` and all library files

#### Scenario: Comparison by the prev build
- **WHEN** the `prev` build of the previous commit exists and the result files of the current build are compared with it
- **THEN** every result file is byte-identical, or the commit names the file as an intended change

#### Scenario: No AB3 mode in blitz2
- **WHEN** `blitz2` is started with `-s SOURCE -e EXE`
- **THEN** it behaves as before the AB3 work (the argument is taken as the project name for the editor), with no AB3 mode
