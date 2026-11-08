# Spec Delta

## Purpose

Gives the compiler a warning level below an error: a condition the user
should know about that does not stop the compile or change its result,
and an option that turns warnings into errors.

## ADDED Requirements

### Requirement: A warning does not stop the compile
A warning SHALL be shown to the user and the compile or the load of the options SHALL go on; a warning SHALL NOT change the executable, the `.xtra` files or any other result. In direct mode (calculator) warnings SHALL be silent, as errors are.

#### Scenario: Warning, compile goes on
- **WHEN** a source is compiled whose residents give a warning and the option "warnings are errors" is off
- **THEN** the warning is shown, the compile finishes and the executable is the same as without the check

### Requirement: Warnings can be turned into errors
With the option "warnings are errors" on, a warning SHALL behave exactly as an error with the same text: the same requester, the same line, the same abort. The option SHALL be settable from the editor's second options requester (the check box "Warnings only", selected by default; deselected means warnings are errors) and from the command line (`-Werror`).

#### Scenario: Option on
- **WHEN** a condition that warns is met with the check box deselected
- **THEN** the compiler reports the same message as an error and stops as for an error

### Requirement: The option is stored without changing the .xtra format
The option SHALL be stored in the `.xtra` file in the word that was unused before (offset +24) and SHALL NOT change the `.xtra` file of a source whose option is untouched.

#### Scenario: Old .xtra files
- **WHEN** a source with an `.xtra` written before this change (the word is `$0080`) is loaded
- **THEN** warnings stay warnings, and saving it writes the same file

### Requirement: The user sees a warning
In the editor a warning SHALL be shown through the editor's message requester with the prefix `Warning: `, without aborting the compile or the load. In a `blitz2-ab3 -s ... -e ...` run it SHALL be printed on the console as `SOURCE: warning: MESSAGE` (with `:LINE` after the source name when it has a line).

#### Scenario: Editor
- **WHEN** a warning is raised outside a compile
- **THEN** the requester shows `Warning: MESSAGE`; after OK the load goes on

#### Scenario: Command line
- **WHEN** a source with clashing residents is compiled with `blitz2-ab3 -s prog.ab3 -e RAM:prog`
- **THEN** the console shows `prog.ab3: warning: Clash in Residents: #NAME (...)`, the executable is written and the return code is 0

### Requirement: Clashing resident constants warn
After the residents are loaded, a constant name defined more than once in the residents with different values SHALL give the warning `Clash in Residents: #NAME`, once per name; a name defined again with the same value SHALL NOT. The first definition SHALL stay the one the compiler uses. Types and macros SHALL keep the error "Clash in Residents".

#### Scenario: Two residents, different values
- **WHEN** one resident defines `#c=1` and another `#c=2`
- **THEN** one warning `Clash in Residents: #c` is shown and the program using `#c` gets 1

#### Scenario: Same value
- **WHEN** two residents both define `#c=1`
- **THEN** no message is shown

#### Scenario: With the option on
- **WHEN** the same residents are loaded with warnings as errors
- **THEN** "Clash in Residents" is an error, the residents are dropped as for a clashing type, and no `#c` is defined
