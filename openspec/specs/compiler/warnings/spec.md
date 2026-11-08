# compiler/warnings Specification

## Purpose
Gives the compiler a warning level below an error: a condition the user
should know about that does not stop the compile or change its result.
Every warning has a name and can be disabled or turned into an error.

## Requirements

### Requirement: Named warnings, enabled and error sets
Every warning SHALL have a name (the first: `resclash`). The compiler SHALL keep two sets of warnings: the enabled warnings and the warnings that are errors; a disabled warning SHALL never be an error. Without a setting all warnings SHALL be enabled and none SHALL be an error.

#### Scenario: Disabled warning
- **WHEN** a condition that warns is met and its warning is disabled
- **THEN** nothing is shown and the compile or load goes on as without the check

#### Scenario: Disabled and error
- **WHEN** a warning is in the error set but not enabled
- **THEN** it is silent, not an error

### Requirement: A warning does not stop the compile
An enabled warning that is not an error SHALL be shown to the user and the compile or the load of the options SHALL go on; a warning SHALL NOT change the executable, the `.xtra` files or any other result. In direct mode (calculator) warnings SHALL be silent, as errors are.

#### Scenario: Warning, compile goes on
- **WHEN** a source is compiled whose residents give an enabled warning that is not an error
- **THEN** the warning is shown, the compile finishes and the executable is the same as without the check

### Requirement: A warning can be an error
A warning in the error set SHALL behave exactly as an error with the same text: the same requester, the same line, the same abort.

#### Scenario: Warning that is an error
- **WHEN** a condition that warns is met and its warning is enabled and in the error set
- **THEN** the compiler reports the same message as an error and stops as for an error

### Requirement: The editor's setting
The editor's second options requester SHALL have one setting for all warnings with three states: off (none enabled), on (all enabled, none an error) and errors (all enabled, all errors). A click SHALL step it off, on, errors, off; Cancel SHALL restore it with the other options.

#### Scenario: Stepping the setting
- **WHEN** the setting shows ON and the user clicks it
- **THEN** it shows ERRORS; after OK warnings are errors for the source

### Requirement: The setting is stored without changing the .xtra format
The setting SHALL be stored in the `.xtra` file in the word that was unused before (offset +24): `$0080` on, `$0000` off, `$0180` errors (read by bits: bit 7 clear is off, bits 7 and 8 set is errors). The `.xtra` file of a source whose setting is untouched SHALL be saved byte-identical.

#### Scenario: Old .xtra files
- **WHEN** a source with an `.xtra` written before this change (the word is `$0080`) is loaded
- **THEN** the setting is on, and saving it writes the same file

### Requirement: The user sees a warning
In the editor an enabled warning SHALL be shown through the editor's message requester with the prefix `Warning: `, without aborting the compile or the load. In a `blitz2-ab3 -s ... -e ...` run it SHALL be printed on the console as `SOURCE: warning: MESSAGE [NAME]` (with `:LINE` after the source name when it has a line).

#### Scenario: Editor
- **WHEN** a warning is raised outside a compile
- **THEN** the requester shows `Warning: MESSAGE`; after OK the load goes on

#### Scenario: Command line
- **WHEN** a source with clashing residents is compiled with `blitz2-ab3 -s prog.ab3 -e RAM:prog`
- **THEN** the console shows `prog.ab3: warning: Clash in Residents: #NAME (...) [resclash]`, the executable is written and the return code is 5

### Requirement: Clashing resident constants warn
After the residents are loaded, a constant name defined more than once in the residents with a value different from its first definition SHALL give the warning `resclash` with the text `Clash in Residents: #NAME`, once per name, at most 8 names followed by one `Clash in Residents: N more`; a name defined again with the same value SHALL NOT warn. The first definition SHALL stay the one the compiler uses. Types and macros SHALL keep the error "Clash in Residents".

#### Scenario: Two residents, different values
- **WHEN** one resident defines `#c=1` and another `#c=2`
- **THEN** one warning `Clash in Residents: #c` is shown and the program using `#c` gets 1

#### Scenario: Same value
- **WHEN** two residents both define `#c=1`
- **THEN** no message is shown

#### Scenario: Many clashes
- **WHEN** two residents define 10 constants with the same names and different values
- **THEN** 8 warnings name the first 8 constants and one says `Clash in Residents: 2 more`

#### Scenario: Clash as an error
- **WHEN** the same residents as in the first scenario are loaded with `resclash` as an error
- **THEN** "Clash in Residents: #c" is an error, the residents are dropped as for a clashing type, and no `#c` is defined
