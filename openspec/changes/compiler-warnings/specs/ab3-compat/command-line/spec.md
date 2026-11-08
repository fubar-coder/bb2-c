# Spec Delta

## Purpose

Adds the options `-w` and `-werr` and the console form of a warning to
the command line of `blitz2-ab3`.

## ADDED Requirements

### Requirement: Warnings on the console
A warning SHALL be printed on the console as `SOURCE: warning: MESSAGE [NAME]` (`SOURCE:LINE: warning: MESSAGE [NAME]` when it has a line) and SHALL NOT change the executable. A warning that is an error SHALL be printed as an error outside the compile (`SOURCE: MESSAGE`) and give the return code 10.

#### Scenario: Warning without options
- **WHEN** a source whose residents clash in a constant is compiled with `-s` and `-e`
- **THEN** the console shows the warning line, the executable is written and the return code is 5

#### Scenario: Warning as an error
- **WHEN** the same compile is run with `-werr resclash`
- **THEN** the console shows the error line, no executable is written and the return code is 10

### Requirement: Return code after warnings
The return codes SHALL be those of AmigaDOS (`dos/dos.h`). A compile that writes the executable SHALL end with `RETURN_WARN` (5) when at least one warning was printed and with `RETURN_OK` (0) when none was. An error SHALL give its own return code also after warnings: `RETURN_ERROR` (10) for a compile error, `RETURN_FAIL` (20) for an argument or set-up error.

#### Scenario: Compile without warnings
- **WHEN** a source without warnings is compiled with `-s` and `-e`
- **THEN** the executable is written and the return code is 0

#### Scenario: Warning and a compile error
- **WHEN** a source whose residents clash in a constant and that has a compile error is compiled with `-s` and `-e`
- **THEN** the console shows the warning line and the error line, no executable is written and the return code is 10

### Requirement: The options -w and -werr
`-w LIST` SHALL set the enabled warnings and `-werr LIST` the warnings that are errors. A LIST SHALL be comma-separated items `all`, `none`, `NAME` (add) and `-NAME` (remove), applied left to right to the source's setting; several options SHALL be applied in order. The options SHALL be case-insensitive like the other options, listed in the usage text with the warning names, and apply in BB2 and AB3 mode.

#### Scenario: Warning disabled
- **WHEN** a source whose residents clash in a constant is compiled with `-w -resclash`
- **THEN** no warning is printed, the executable is written and the return code is 0

#### Scenario: Usage lists the options
- **WHEN** `blitz2-ab3 -h` is run
- **THEN** the text lists `-w`, `-werr` and the warning `resclash`, and the return code is 0

### Requirement: Errors in a warning list
An unknown warning name, an empty item or a missing list SHALL be an argument error: a message naming the problem and the usage, return code 20, nothing compiled.

#### Scenario: Unknown warning name
- **WHEN** `blitz2-ab3 -s prog.ab3 -e RAM:prog -w nosuch` is run
- **THEN** a message naming `nosuch` and the usage are printed, nothing is compiled, and the return code is 20
