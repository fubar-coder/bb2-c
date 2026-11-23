## ADDED Requirements

### Requirement: The compile jobs of a cell have names
Every compile job of a cell SHALL have a name: the driver's job `j000`; in
a cell with `editor-run = "on"` its run `r001`; a data source of
`basic-compile` its file name relative to `ID.test/`; a job of `jobline`
(`job=`, `run=`, `menu=`, `resident=`) its ID.

#### Scenario: A data source is named by its file
- **WHEN** a test declares `"victim.bas" = "basic-compile"` behind another data source of `basic-compile`
- **THEN** the name of its compile job is `victim.bas`, whatever its position in `[data]`

### Requirement: The IDs of the jobline jobs are checked
A jobline ID SHALL match `[a-z][a-z0-9_]*`, have at most 15 characters,
be unique among the jobline elements of a cell (a `resident=` element
counts once), and SHALL NOT be `j000`, `b` followed by three digits,
`log`, `inject`, or `r001` in a cell with `editor-run = "on"`; otherwise
the configuration SHALL be refused naming file, line and ID.

#### Scenario: A jobline job named like the driver's
- **WHEN** a test sets `jobline = ["job=j000|@DRIVER@|CORPUS:out/again"]`
- **THEN** the configuration is refused naming the file, the line and `j000`

#### Scenario: Two jobline elements with one ID
- **WHEN** a test sets `jobline = ["job=j001|@DRIVER@|CORPUS:out/a", "menu=j001|9"]`
- **THEN** the configuration is refused naming `j001`

### Requirement: The log block of a named job
The query `compile.log.NAME` SHALL be the block of the job `NAME` in
tedstub's log of the session that compiled it (a data source's own build
session where it has one): its lines from the first that belongs to it up
to its last line that carries its ID, with the size removed from a
`RESULT ID ok SIZE` line as in `compile.log`. A block without lines SHALL
be not captured. The predicates SHALL be those of `compile.log`.

#### Scenario: Two jobs, two blocks
- **WHEN** the driver's job j000 logs `GUARD\tj000\tok` and the jobline job j001 logs `GUARD\tj001\tok`
- **THEN** `compile.log.j000` contains the first line and not the second, and `compile.log.j001` the second and not the first

#### Scenario: A job that did not run
- **WHEN** a session ended before the jobline job j002 and an assertion asks `compile.log.j002` to contain a line
- **THEN** the cell fails naming the assertion as not captured

### Requirement: The block of a data source does not show its position
In the block of a data source its internal job ID SHALL be written as its
file name and its staged source as `@DATA:FILE@`. A `NAME` that is no job
of the test SHALL be refused naming file, line and query.

#### Scenario: The block of a data source
- **WHEN** the data source `victim.bas` is compiled as the internal job b001 and the assertion is `"compile.log.victim.bas" = "contains \"RESULT\tvictim.bas\tok\""`
- **THEN** the assertion holds

#### Scenario: An unknown name
- **WHEN** a test asserts `compile.log.j009` and has no job j009
- **THEN** the configuration is refused naming the file, the line and the query

### Requirement: The compile result of a named jobline job
The query `compile.NAME` SHALL be the compile result of the `job=` or
`resident=` job `NAME` of `jobline` in the terms of `compile` (`ok`,
`error LINE MESSAGE` with the line of the compiled source, another result
of its `RESULT` line, or `none`), with the predicates of `compile` and no
default assertion. For the driver's job it SHALL be refused naming
`compile`, for a data source naming `data.FILE`, and for a `run=` or
`menu=` step naming `compile.log.NAME`.

#### Scenario: A second compile of the driver
- **WHEN** a test asserts `"compile.j001" = "== ok"` and its jobline job j001 compiles `@DRIVER@` without error
- **THEN** the assertion holds

#### Scenario: The driver's result under its name
- **WHEN** a test asserts `compile.j000`
- **THEN** the configuration is refused naming `compile`

#### Scenario: A data source's result under its name
- **WHEN** a test asserts `compile.victim.bas`
- **THEN** the configuration is refused naming `data.victim.bas`

### Requirement: The unnamed compile queries
The queries `compile`, `compile.log` and `compile.inject` SHALL keep their
meaning: the driver's compile result with its default `== ok`, the whole
log (the compiler's output, then tedstub's log), and inject's report of
the compiler process. Inject's report SHALL have no query per job.

#### Scenario: The whole log with named jobs
- **WHEN** a cell has the jobs j000 and j001 and asserts `compile.log` to contain a line of j001's block
- **THEN** the assertion holds as without named jobs

### Requirement: The staged source of a data source in a job line
The placeholder `@DATA:FILE@` in an element of `jobline` SHALL stand for
the tokenised source of the data source `FILE` (action `basic-compile`) on
the cell's volume. In another setting, or for a `FILE` that is no data
source of `basic-compile` of the test, it SHALL be an error naming file,
line and placeholder.

#### Scenario: The victim of a run from the editor
- **WHEN** a test with the data source `victim.bas` sets `jobline = ["run=r001|@DATA:victim.bas@|RAM:con|100"]`
- **THEN** the cell's job list runs the staged source of `victim.bas` as r001, whatever its position in `[data]`

#### Scenario: A file that is not a data source
- **WHEN** a test sets `jobline = ["run=r001|@DATA:other.bas@|RAM:con"]` and declares no data source `other.bas`
- **THEN** the configuration is refused naming the file, the line and `@DATA:other.bas@`

#### Scenario: Outside a job line
- **WHEN** a test sets `files = ["x.bb2|@DATA:victim.bas@"]`
- **THEN** the configuration is refused naming the placeholder
