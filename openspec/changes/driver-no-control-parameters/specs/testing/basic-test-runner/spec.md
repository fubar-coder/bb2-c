## MODIFIED Requirements

### Requirement: The driver takes no parameters
The name of the result file and the selected cases SHALL be compiled into the driver; the driver SHALL take no control data as parameters, also in the run of all cases. Arguments of the run setting `runargs` are for the tested behaviour; the driver SHALL ignore them.

#### Scenario: The run of all cases
- **WHEN** all cases of a source run
- **THEN** the program is the driver of all cases, started without arguments

#### Scenario: A test passes arguments
- **WHEN** a test sets `runargs` to a 306-character string (`wbstartuplib-clistring-overflow`)
- **THEN** the program gets the string as its argument and runs its compiled case list as without it
