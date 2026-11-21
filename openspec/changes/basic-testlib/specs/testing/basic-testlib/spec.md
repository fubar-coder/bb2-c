# Spec Delta

## Purpose

Defines the test library with which a Blitz test program judges itself
in setup / act / verify form: the form of a test source, its cases,
checks and preconditions, the result file, and what the library keeps
unchanged for the tested program, for the original compilers and the port.

## ADDED Requirements

### Requirement: A test source holds declarations and blocks
A test source SHALL consist of declarations (including the definitions of
`Statement` and `Function` procedures), at most one batch init, one or more
cases and at most one batch end, and SHALL NOT be a main program of its own:
its text is compiled as part of the driver the runner generates
(`testing/basic-test-runner`), which calls its blocks. Executable code
outside the blocks SHALL be refused at build time with file and line.

#### Scenario: A statement outside the blocks
- **WHEN** a test source calls `LoadShape` or has a `Dim` outside its batch init, cases and batch end
- **THEN** the build-time check of the source reports it with file and line

#### Scenario: A helper procedure
- **WHEN** a test source defines `Function.l chipfree{}` outside the blocks and a case calls it
- **THEN** the source is accepted and the function runs when the case calls it

### Requirement: A case is a subroutine with a name
A case SHALL be a block that starts with the case macro naming it and ends
with the end macro; the driver calls it as a subroutine. A case without its
end macro SHALL fail the compile of the driver. A case name SHALL match
`[a-z][a-z0-9_]*`, SHALL contain no `__`, SHALL NOT end in `_originals`,
SHALL NOT be a token of any library set, and SHALL be unique within the
source; the build-time check SHALL refuse any other name.

#### Scenario: A missing end of a case
- **WHEN** a case of a source has no end macro
- **THEN** compiling the driver fails

#### Scenario: A name that is a token
- **WHEN** a case is named `free`
- **THEN** the build-time check refuses it naming the case and the token

### Requirement: Every check is identified
Every check SHALL be identified by its namespace and its number in it,
counted from 0 in source order, and MAY carry a name; a check that a run
configuration names SHALL have one. The namespace SHALL be the case, the
procedure, `init` (batch init) or `end` (batch end) the check stands in.
The identification SHALL be the same in every compile and in every run of
the same source.

#### Scenario: A check is added to another case
- **WHEN** a check is added to case a
- **THEN** the identifications of the checks of case b do not change

#### Scenario: A check in a loop
- **WHEN** a check in a loop runs three times
- **THEN** the result file holds three entries with the same identification

#### Scenario: A check in a procedure
- **WHEN** cases a and b both call a procedure that holds a check
- **THEN** the result file holds an entry of that check for case a and one for case b, each with the procedure's namespace

### Requirement: No checks inside the test's own macros
A check, a precondition or an announcement SHALL NOT stand inside a macro
definition of the test source; the build-time check SHALL refuse it with
file and line. A check given as an argument of a macro use SHALL be allowed.

#### Scenario: A check in a macro
- **WHEN** a test source defines `Macro M` holding `!CheckEq{a,1}` before `End Macro`
- **THEN** the build-time check refuses it naming file and line

### Requirement: A check records expected and actual value
A check SHALL compare the actual value with the expected value written in
the program and SHALL record the running case, the check's identification,
the verdict (ok or fail), the expected and the actual value. A failed check
SHALL NOT end the case: the following checks run. A failed check in the
batch init SHALL fail every selected case.

#### Scenario: A failed check
- **WHEN** a check expects 16 and gets 0
- **THEN** the result file holds that check with verdict fail, expected 16 and actual 0, and the next check of the case runs

### Requirement: A failed precondition marks the test broken
A precondition SHALL be checked like a check, but when it fails the rest of
its case SHALL NOT run and the case SHALL be broken; in the batch init every
selected case SHALL be broken and none SHALL run. A broken result SHALL
never count as a reproduced original bug.

#### Scenario: Setup cannot allocate
- **WHEN** a precondition in a case finds that an allocation failed
- **THEN** the case is broken, its later statements do not run, and the next case runs

#### Scenario: Batch init fails
- **WHEN** a precondition fails in the batch init
- **THEN** no case runs and every selected case is reported as broken

### Requirement: An expected runtime error is announced
A case whose correct result is a runtime error SHALL announce the expected
error (its message) before the statement that raises it. The library SHALL
record the announcement in the result file, so that the run can match it
with the error the run shows.

#### Scenario: The error comes as announced
- **WHEN** a case announces "Shape too Big" and the next statement raises that runtime error
- **THEN** the result file holds the announcement as the last entry of the case, then the incomplete entry and the end of the run

### Requirement: Results go to a file of their own
The library SHALL write its results only to the file the driver names,
never to the program's standard output or console. In Amiga mode each entry
SHALL be written when it is made, so that the entries before a crash stay;
while the program is in Blitz mode or has multitasking or interrupts off,
entries SHALL be kept and written at the next entry in Amiga mode or at the
end.

#### Scenario: The program prints
- **WHEN** a test program prints with `NPrint` and its checks run
- **THEN** the program's output holds only what it printed and the result file only the library's entries

#### Scenario: The program crashes
- **WHEN** the program crashes after its second check in Amiga mode
- **THEN** the result file holds the entries up to the second check

#### Scenario: A check in Blitz mode
- **WHEN** a check runs after `BLITZ` and the program returns to Amiga mode with `QAMIGA`
- **THEN** the entry is in the result file after the return, in order

### Requirement: The end of a case and of the run are marked
The library SHALL write an end entry when a case ends, and an end entry for
the run when the program ends; when the program ends while a block is open
(a runtime error, `End` in a case), it SHALL write an incomplete entry
before the end of the run. A case without its end entry SHALL mean that the
case did not run to its end.

#### Scenario: End inside a case
- **WHEN** a case executes `End` after its first check
- **THEN** the result file has no end entry for that case

#### Scenario: Normal end
- **WHEN** the program runs all its cases to their end
- **THEN** every case has its end entry and the last entry is the end of the run

### Requirement: The program does not see the library's control data
The library SHALL take the name of its result file and its other control
data from the driver, not from the command line or the Workbench arguments:
`NumPars`, `Par$` and the Workbench arguments SHALL give the same values as
in a run without the library.

#### Scenario: A test of the argument functions
- **WHEN** a test started with the argument "x" reads `NumPars` and `Par$(1)` in a case
- **THEN** it gets 1 and "x"

#### Scenario: A Workbench start
- **WHEN** a test is started from the Workbench
- **THEN** its result file is written as with a start from the CLI

### Requirement: The library keeps the program's state
Between its start and the end of the run the library SHALL NOT allocate or
free memory, and its routines SHALL keep the registers the library register
contract names (`libs/register-contract`) and in addition `d2` to `d7` and
`a2`.

#### Scenario: Free memory around a check
- **WHEN** a case reads the free memory, runs a check, and reads it again
- **THEN** both values are equal

#### Scenario: Registers around a check
- **WHEN** the register test of the library calls each routine with known values in `d2`-`d7` and `a2`-`a6`
- **THEN** they are unchanged after the call

### Requirement: The library works with the originals and with the port
The library and its macros SHALL compile and run with the original blitz2
1.95, the CD's blitz2 2.10 and the CD's libraries as well as with the port's
compiler and the C libraries, with the same result entries for the same
behaviour.

#### Scenario: Same source, three compilers
- **WHEN** a test source whose checks all pass is compiled by 1.95, by 2.10 and by the port
- **THEN** the three result files hold the same entries apart from actual values the checks do not compare
