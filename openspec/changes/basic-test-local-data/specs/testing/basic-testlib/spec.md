## MODIFIED Requirements

### Requirement: The library keeps the program's state
Between its start and the end of the run the library SHALL NOT allocate or
free memory, and its routines SHALL keep the registers the library register
contract names (`libs/register-contract`) and in addition `d2` to `d7` and
`a2`. The one exception SHALL be `TLRunProgram`: what the system and the
started program allocate while it runs is theirs, and the library opens
and closes the output file of the program around the start.

#### Scenario: Free memory around a check
- **WHEN** a case reads the free memory, runs a check, and reads it again
- **THEN** both values are equal

#### Scenario: Registers around a check
- **WHEN** the register test of the library calls each routine with known values in `d2`-`d7` and `a2`-`a6`
- **THEN** they are unchanged after the call

#### Scenario: Registers around a started program
- **WHEN** the register test calls `TLRunProgram` with known values in `d2`-`d7` and `a2`-`a6` and a program that returns
- **THEN** they are unchanged after the call

## ADDED Requirements

### Requirement: A case starts a program
A case SHALL be able to start a program with `!RunProgram`, under Kickstart 1.3 and later, with the original compilers and with the port. Where the system gives no return code of a command line, the entry SHALL say so (`-`) instead of a value. A run outside a block, or in Blitz mode, SHALL be refused with a `broken` entry.

#### Scenario: Kickstart 1.3
- **WHEN** a case on the machine `a500-68010` starts a child that prints a line
- **THEN** the output file holds the line and the result file `run` and `runend`, with the return code or `-`

#### Scenario: A run in Blitz mode
- **WHEN** a case calls `!RunProgram` after `BLITZ`
- **THEN** the file holds `broken` with the reason `run in Blitz mode` and no program starts

### Requirement: The result of a run is the library's
The `TLRun` that `TLRunProgram` gives and the string of its field `printed` SHALL be one static block of the library; a later `!RunProgram` SHALL overwrite it, so the address of an earlier run then shows the later run. The string of `printed` SHALL be a Blitz string in the library's static memory (its length as a long before the characters, a 0 behind them); the case SHALL only read the fields, and the library's reference SHALL say so.

#### Scenario: Two runs in one case
- **WHEN** a case keeps `*a.TLRun` of a first run, copies `x$=*a\printed`, and starts a second program
- **THEN** `x$` keeps the first output and `*a\printed` shows the second

#### Scenario: Checks on a run in BASIC
- **WHEN** a case runs `*r.TLRun=!RunProgram{"@TESTDIR@/child"}` and the child prints `edit$ 0` and returns 20
- **THEN** `!CheckEq{*r\rc,20}` and `!CheckStrEq{*r\printed,"edit$ 0"+Chr$(10)}` pass
