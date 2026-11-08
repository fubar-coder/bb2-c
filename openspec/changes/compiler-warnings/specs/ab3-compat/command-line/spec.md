# Spec Delta

## Purpose

Adds the option `-Werror` and the console form of a warning to the
command line of `blitz2-ab3`.

## ADDED Requirements

### Requirement: Warnings on the console, -Werror
A warning SHALL be printed on the console as `SOURCE: warning: MESSAGE` (`SOURCE:LINE: warning: MESSAGE` when it has a line) and SHALL NOT change the return code (0 when the executable was written). With `-Werror` every warning SHALL be an error: the console form of an error (`SOURCE[:LINE]: MESSAGE`) and the return code 10. `-Werror` SHALL be accepted in any position, case-insensitively like the other options, and be listed in the usage text; it applies in BB2 and AB3 mode.

#### Scenario: Warning without the option
- **WHEN** a source whose residents clash in a constant is compiled with `-s` and `-e`
- **THEN** the console shows the warning line, the executable is written and the return code is 0

#### Scenario: Warning with -Werror
- **WHEN** the same compile is run with `-Werror`
- **THEN** the console shows the error line, no executable is written and the return code is 10

#### Scenario: Usage lists the option
- **WHEN** `blitz2-ab3 -h` is run
- **THEN** the text lists `-Werror` and the return code is 0
