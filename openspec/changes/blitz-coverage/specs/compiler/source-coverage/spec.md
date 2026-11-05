# compiler/source-coverage Specification

## Purpose

Tells which statements of a compiled Blitz program ran and how often: the
compilers can build a program that counts its statements, and the counts
become an lcov report on the host, in BB2 mode and in AB3 mode.

## ADDED Requirements

### Requirement: Coverage is an option, off by default
`blitz2-ab3` SHALL accept `-coverage` (short form `-cov`) with `-s` and `-e`; without it every result of a compile SHALL be byte-identical to a compile of the same source by the previous state of the repository.

#### Scenario: Normal compile unchanged
- **WHEN** a source is compiled without `-coverage`
- **THEN** the executable is byte-identical to the one the previous commit's build writes, and no coverage file is written

#### Scenario: Option given
- **WHEN** `blitz2-ab3 -s prog.ab3 -e RAM:prog -coverage` is run on a source without errors
- **THEN** `RAM:prog` and the statement map `RAM:prog.covmap` are written and the return code is 0

#### Scenario: Both modes
- **WHEN** the same compile is run with `-mode bb2` and with `-mode ab3`
- **THEN** both write an executable and a map

### Requirement: Independent of runtime error checking
The coverage mode SHALL work with and without runtime error checking (`-r`), and SHALL NOT change what error checking reports.

#### Scenario: Release build with coverage
- **WHEN** a source is compiled with `-r -coverage`
- **THEN** the program has no runtime error checks, counts its statements, and writes its counts when it ends

#### Scenario: Error checking on
- **WHEN** a program with a division by zero is compiled with `-coverage` and error checking on
- **THEN** it reports the error exactly as without `-coverage`, and the counts up to the error are written

### Requirement: Statements are counted
A program compiled with `-coverage` SHALL count, for every BASIC statement of its source that produces code, how many times the statement was executed; inline assembler lines SHALL not be counted and SHALL generate the same code as without the option. The program SHALL write the counts when it ends, also when it ends through `End`, a runtime error or the user closing it.

#### Scenario: Loop
- **WHEN** a program has a `For`/`Next` loop of 10 iterations with a body of one statement per line and ends normally
- **THEN** the counts of the body's statements are 10, the loop's first statement 1

#### Scenario: Not executed
- **WHEN** a statement is never reached (after an `End`, or in a branch not taken)
- **THEN** its count is 0 and it still appears in the report

#### Scenario: Inline assembler
- **WHEN** a program with inline assembler (a compare on one line and a branch on the next) is compiled with `-coverage`
- **THEN** it runs as without the option, and its assembler lines appear in the report without counts

#### Scenario: Kickstart 1.3
- **WHEN** a program compiled with `-coverage` runs under Kickstart 1.3
- **THEN** it runs as without the option and writes its counts

### Requirement: Counts are merged over runs
The counts of a program SHALL be added to those already in its count file when the file belongs to the same compile, so that several runs (or several test inputs) accumulate; a file of another compile SHALL be replaced.

#### Scenario: Two runs
- **WHEN** a program that executes line 5 once is run twice
- **THEN** the count file says line 5 ran twice

#### Scenario: Recompiled
- **WHEN** the source changes, is compiled again with the same `-e`, and run
- **THEN** the old counts are not mixed into the new file

### Requirement: The statement map names file and line
With `-coverage` the compiler SHALL write, next to the executable, a map of every counted statement with its source file and line, and of every `Statement` and `Function` with its name, file and first line. Lines inside an include SHALL be given in that include's file with the line number in that file.

#### Scenario: Line numbers
- **WHEN** statement 3 of a main source starts on line 7
- **THEN** the map names the main source and line 7 for it

#### Scenario: Include
- **WHEN** a statement is on line 4 of an included file
- **THEN** the map names the include's file and line 4, not the line of the `INCLUDE` statement

#### Scenario: Several statements on a line
- **WHEN** a line holds `a=1 : b=2`
- **THEN** both statements map to that line

#### Scenario: Macros
- **WHEN** a macro is used on line 20
- **THEN** statements produced by its expansion map to line 20 or are not counted, and never to a line of the macro's definition that did not run

### Requirement: lcov report on the host
The tool `blitzcov` SHALL turn a statement map and a count file into an lcov tracefile, with `SF` for each source file, `DA:line,count` for each line with a counted statement, and `FN`/`FNDA`/`FNF`/`FNH` for each procedure, plus `LF`/`LH`.

#### Scenario: Line count with several statements
- **WHEN** a line has statements with counts 1, 10 and 1
- **THEN** its `DA` count is 10, the largest of them

#### Scenario: Procedure
- **WHEN** a `Statement` defined on line 3 is called 4 times
- **THEN** the report has `FN:3,name` and `FNDA:4,name`

#### Scenario: Several runs
- **WHEN** `blitzcov` is given the maps and counts of several programs or runs
- **THEN** the lines of one source file are merged into one `SF` record by adding their counts

#### Scenario: Broken input
- **WHEN** the count file does not belong to the map (another compile, or truncated)
- **THEN** `blitzcov` reports it and writes no report

### Requirement: Test programs leave per-test reports
The emulator test runners of `tests/ab3/cli` and the corpus SHALL be able to compile with the coverage mode and leave the lcov report of a test at `<build>/coverage/lcov/<test>.info`.

#### Scenario: A measured test
- **WHEN** a corpus test is run in the coverage configuration
- **THEN** `<build>/coverage/lcov/<test>.info` exists and names the test's Blitz source files

#### Scenario: Results unchanged
- **WHEN** the same test is run without the coverage configuration
- **THEN** no coverage file is written and the test's checks are the same
