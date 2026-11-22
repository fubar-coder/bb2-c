# Spec Delta

## Purpose

Says how a program compiled from the command line can be made to end on Ctrl-C at its next console input or output, also without runtime error checking, and that programs compiled without this option are not changed.

## ADDED Requirements

### Requirement: The command-line switch turns the console break on
`blitz2-ab3` SHALL accept the switch `-ctrlcbreak` (in any case) for a BB2-mode source. A program compiled with it SHALL react to Ctrl-C at every call of Print, NPrint, Edit$ and Edit as this spec says, also when it is compiled without runtime error checking.

#### Scenario: Release executable with the switch
- **GIVEN** a BB2-mode source compiled with `blitz2-ab3 -s SOURCE -e EXE -r -ctrlcbreak`
- **WHEN** the program sends itself Ctrl-C (`SetSignal_ $1000,$1000`) and then executes `NPrint "signalled"` on the console
- **THEN** the console shows `signalled`, then `***Break`, and the program ends with return code 20 without executing its next statement

#### Scenario: Usage
- **WHEN** `blitz2-ab3 -h` is run
- **THEN** the usage lists `-ctrlcbreak`

### Requirement: Only the console reacts
The check SHALL act only when the call's channel is the default one: the console output for Print and NPrint, the console input for Edit$ and Edit. Output to a window, a bitmap or a file and input from them SHALL never end the program.

#### Scenario: Output to a file
- **GIVEN** a program compiled with the switch whose output channel is a file (FileOutput)
- **WHEN** Ctrl-C is set and the program prints
- **THEN** the program goes on, and the signal stays set

#### Scenario: Back on the console
- **GIVEN** the same program after DefaultOutput
- **WHEN** it prints with Ctrl-C still set
- **THEN** it ends with `***Break` and return code 20

### Requirement: One check per call, after the call
The check SHALL run once per call of Print, NPrint, Edit$ or Edit, after the call has done its input or output; it SHALL not check per character or per statement.

#### Scenario: Input
- **GIVEN** a program compiled with the switch reading `a$=Edit$(80)` from the console
- **WHEN** Ctrl-C is set before the line is entered
- **THEN** the line is read first, then the program ends with `***Break` and return code 20

### Requirement: The end is the program's normal end
On a break the signal SHALL be cleared, `***Break` and a newline SHALL be written to the console, and the program SHALL end as at `End` (the libraries' cleanup runs) with return code 20.

#### Scenario: Cleanup
- **WHEN** a program compiled with the switch that has opened a file ends by Ctrl-C at an NPrint
- **THEN** the file is closed as at `End` and the return code is 20

### Requirement: A program that consumes the signal keeps working
A program that tests or waits for Ctrl-C itself and so clears the signal before its next console call SHALL not be ended by the check.

#### Scenario: Own test
- **GIVEN** a program compiled with the switch that calls `SetSignal_ 0,$1000` after setting the signal
- **WHEN** it prints on the console
- **THEN** it goes on

### Requirement: Runtime error checking keeps its own check, Runerrsoff regions get the release check
With runtime error checking on and the switch given, a console call that gets the check of runtime error checking SHALL behave as without the switch, and a console call between `Runerrsoff` and `Runerrson` SHALL get the check of this spec.

#### Scenario: Runtime error checking on
- **WHEN** a source is compiled with the switch and with runtime error checking, and the program prints on the console with Ctrl-C set
- **THEN** it ends with `***Break` and return code 20, as without the switch

#### Scenario: Runerrsoff region
- **WHEN** the same program prints on the console between `Runerrsoff` and `Runerrson` with Ctrl-C set
- **THEN** it ends with `***Break` and return code 20

### Requirement: AB3 mode refuses the switch
For a source in AB3 mode `blitz2-ab3` SHALL refuse `-ctrlcbreak` with a message and return code 20, and write no executable.

#### Scenario: AB3-mode source
- **WHEN** `blitz2-ab3 -s PROG.ab3 -e EXE -ctrlcbreak` is run
- **THEN** it prints that the option needs BB2 mode and ends with return code 20

### Requirement: Compiles without the switch do not change
An executable compiled without the switch (by TED, blitz2 or blitz2-ab3) SHALL be byte-identical to one compiled before the switch existed, with and without runtime error checking.

#### Scenario: Release executable without the switch
- **WHEN** a source is compiled with `blitz2-ab3 -r` and without `-ctrlcbreak`
- **THEN** the executable equals the previous commit's, and the program does not react to Ctrl-C
