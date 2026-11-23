## ADDED Requirements

### Requirement: Test-local data are declared in the test's own file
The files of the directory `ID.test/` beside `ID.test.bas` SHALL be declared in the table `data` of `ID.test.toml`, one entry per file by its path relative to `ID.test/`. The table SHALL be refused in a `tests.toml`. A file of `ID.test/` without an entry, and an entry without its file, SHALL be errors of the registration naming file and line.

#### Scenario: An undeclared file
- **WHEN** `ID.test/` holds `child.bas` and `extra.bin` and `ID.test.toml` declares only `child.bas`
- **THEN** the registration refuses the test, naming `extra.bin` and `ID.test.toml`

#### Scenario: Data in a directory's file
- **WHEN** a `tests.toml` holds a table `data`
- **THEN** the registration refuses it, naming the file and the line

### Requirement: Each data file has a build action
An entry of `data` SHALL name a build action: `basic-compile` (a Blitz source compiled in the emulator; the result is named without `.bas`), `copy` (the file as it is) or `mktestdata` (a `NAME.spec` made into `NAME` on the host). An unknown action or an option the action does not take SHALL be an error naming file, line and key.

#### Scenario: An unknown action
- **WHEN** an entry names the action `assemble`
- **THEN** the registration refuses it, naming the file, the line and the value

#### Scenario: The result of a compile
- **WHEN** `child.bas` has the action `basic-compile`
- **THEN** the program finds the executable as `CORPUS:ID.test/child`

### Requirement: The compiler of a data source
The action `basic-compile` SHALL take the options `compiler` (`same`, `port`, `orig195`, `orig210`), `libs` (`same`, `cd`, `clib`) and `runtime-errors` (`same`, `on`, `off`), each `same` by default: `same` is the value of the cell. A data source SHALL be tokenised with the token list of the cell and get the placeholders of a test source, but no check macro.

#### Scenario: The cell's compiler
- **WHEN** `child.bas` has `compiler = "same"` in a test with the cells `port,clib` and `orig195,cd`
- **THEN** the child is compiled by the port with the C libraries in the first cell and by 1.95 with the CD's libraries in the second

#### Scenario: A check macro in a data source
- **WHEN** a data source with the action `basic-compile` holds `!CheckEq{a,1}`
- **THEN** the registration refuses it, naming the data file and the line

### Requirement: Data that need original material
A data file whose build needs original material (a compiler other than the port, or the CD's libraries, by a fixed option) SHALL be refused in a test that has a cell of the side without original material.

#### Scenario: A fixed original compiler on the port side
- **WHEN** a test with the cell `port,clib` declares `compiler = "orig195"` for a data source
- **THEN** the registration refuses it, naming the file, the line and the cell

### Requirement: Built data are in the cell's own CORPUS
Each cell SHALL find the built files of its test under `CORPUS:ID.test/` of its own `CORPUS:`, before its program runs; `@TESTDIR@` SHALL stand for `CORPUS:ID.test` in the test source and the data sources.

#### Scenario: Two cells with different compilers
- **WHEN** two cells of one session compile `child.bas` with their own compilers
- **THEN** each cell's program starts its own executable from `@TESTDIR@/child`

### Requirement: The compile of a data source is judged
The query `data.FILE` SHALL be the compile result of the data source `FILE` in the terms of `compile` (`ok`, `error LINE MESSAGE` with the line of the data source, or `none`), with the default assertion `== ok`. A compile that does not give what its assertion asks SHALL make every case of the cell BROKEN, naming the data file and the result.

#### Scenario: A data source that does not compile
- **WHEN** `child.bas` gives `error 3 Syntax Error` and no assertion names `data.child.bas`
- **THEN** every case of the cell is BROKEN and the report names `child.bas:3` and the message

#### Scenario: A declared compile error of the original
- **WHEN** the deviations of `compiler=orig195` hold `"data.child.bas" = "contains \"Syntax Error\""`
- **THEN** that cell's cases run, judged without the child's executable

### Requirement: The observations of a started program belong to its case
A case that holds a `!RunProgram` SHALL run on its own in the run of all cases. The requesters, the crash or the hang of a program it starts SHALL be judged as observations of that case.

#### Scenario: A requester of a child
- **WHEN** a case starts a child that shows the requester "End of Program"
- **THEN** the query `requester` of that case holds it, and no other case of the source is judged from that run

### Requirement: Queries of a started program
The queries `run.NS.NAME` (`done` it returned, `open` it did not, `not-run`), `run.NS.NAME.rc` (its return code; `CMP integer`, `exists`) and `run.NS.NAME.output` (what it wrote; the string predicates) SHALL judge the program the `!RunProgram` named `NAME` started; they are for what the case cannot check itself (a run that does not return, its output then). The default SHALL be `run.NS.NAME == done` for every one that ran; the return code has no default (the case checks it with `*r\rc`).

#### Scenario: The output of a child that hangs
- **WHEN** a started child prints `cols 0,0,0` and then hangs until the hang check ends the session
- **THEN** `run.NS.NAME` is `open`, `run.NS.NAME.output` holds `cols 0,0,0`, and the cell fails unless an assertion or deviation declares both

#### Scenario: A return code without a default
- **WHEN** a child ends with return code 20 and the configuration asserts `"run.ctrlc.edit.rc" = "== 20"`
- **THEN** that point of the cell passes

### Requirement: Compiled data in the prev comparison
The executables of `basic-compile` data SHALL be compared with the previous commit's compiler wherever the test's driver is compared, under the name `ID/FILE` for the `PREV_INTENDED` entries.

#### Scenario: A compiler change that alters a child
- **WHEN** a commit changes the code the compiler writes for a statement of `child.bas` in a test of `tests/blitz2`
- **THEN** the `_prev` test fails naming `ID/child.bas`, unless an entry `TEST:ID/child.bas` names it
