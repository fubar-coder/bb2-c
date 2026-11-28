# Delta: testing/basic-test-runner

## ADDED Requirements

### Requirement: The query of a compile's warnings
The query `compile.warnings` SHALL be the warnings of the driver's compile,
one line per warning in the order the compiler gave them, each line
`PLACE NAME TEXT` ended by a newline: PLACE the line of the test source the
warning names, `driver:N` for a line N of the driver outside the test
source, the place as the compiler names it for another file, or `-`
without a place; NAME the warning's name, or `-` where the compiler names
none; TEXT the message.

#### Scenario: A warning on the command line
- **WHEN** line 11 of an AB3 test source is `Data.b -1.5,1.0` and its cell of `compiler = "ab3"` compiles it
- **THEN** `compile.warnings` is `11 datafrac Fraction of a Data.b value discarded` followed by a newline

#### Scenario: A warning in the driver's own text
- **WHEN** the compiler names a warning at a driver line that belongs to no line of the test source
- **THEN** its PLACE is `driver:N` with that driver line

### Requirement: The predicates and the default of the warnings query
The predicates of `compile.warnings` SHALL be `==`/`!=` string,
`contains`, `matches`, `exists` (not empty) and `count CMP integer` (the
number of entries). A compile that gave no warning SHALL give the empty
list; a compile that did not run or did not end SHALL leave the query not
captured. The query SHALL have no default assertion.

#### Scenario: The original compiler
- **WHEN** a cell of `compiler = "orig195"` compiles a source on which the port warns
- **THEN** `compile.warnings` is the empty list and `count == 0` holds

#### Scenario: No default
- **WHEN** a test that asserts no query of the warnings is compiled with a warning
- **THEN** its cases are judged as without the warning

### Requirement: The editor's count of further places
The editor's message that counts the places of a warning it did not show
(`NAME: K more`) SHALL be the entry `- NAME K more` of `compile.warnings`,
at the position the compiler gave it.

#### Scenario: The editor's limit
- **WHEN** a BB2 test source with 10 places of `datafrac` on its lines 14 to 23 is compiled by blitz2 through the editor path
- **THEN** `compile.warnings` holds the entries of lines 14 to 21 and then `- datafrac 2 more`, and `count == 9` holds

### Requirement: One warning by its number
The query `compile.warnings.K`, K a decimal number from 1 without leading
zeros, SHALL be the K-th entry of `compile.warnings` without its newline;
for a K beyond the list it SHALL be not captured. Its predicates SHALL be
those of `compile.warnings` but `count`. Another K SHALL be refused naming
file, line and query.

#### Scenario: The second warning
- **WHEN** the warnings are those of lines 9 and 10 and a test asserts `"compile.warnings.2" = "== \"10 datafrac Fraction of a Data.b value discarded\""`
- **THEN** the assertion holds

#### Scenario: A number beyond the list
- **WHEN** the compile gave two warnings and a test asserts `compile.warnings.3` to contain `datafrac`
- **THEN** the cell fails naming the assertion as not captured

#### Scenario: A malformed number
- **WHEN** a test asserts `compile.warnings.0` or `compile.warnings.02`
- **THEN** the configuration is refused naming the file, the line and the query

### Requirement: The warnings of the editor path
In a cell compiled through the editor path, the warnings SHALL be the
WARNING lines of the block of the job `j000` in tedstub's log, read as
`Warning: line N: TEXT [NAME]` (N the driver's line, giving PLACE as the
query `compile` maps a line), as `Warning: NAME: K more` (NAME one of the
compiler's warning names), and otherwise as `Warning: TEXT` with PLACE and
NAME `-`.

#### Scenario: A type warning in the editor
- **WHEN** tedstub logs a line `WARNING`, `j000`, `Warning: line 85: Fraction of a Data.b value discarded [datafrac]` (tab separated) and driver line 85 is line 14 of the test source
- **THEN** the entry is `14 datafrac Fraction of a Data.b value discarded`

### Requirement: The warnings of the command line
In a cell of `compiler = "ab3"`, the warnings SHALL be the lines
`SOURCE: warning: MESSAGE [NAME]` and `SOURCE:LINE: warning: MESSAGE
[NAME]` of the compiler's console output; SOURCE the driver SHALL give
PLACE as in the editor path, another file the PLACE `SOURCE:LINE` as
printed.

#### Scenario: A place in an include
- **WHEN** an included file staged as `CORPUS:src/inc.ab3` gives a warning at its line 3 on the command line
- **THEN** the entry is `CORPUS:src/inc.ab3:3 datafrac Fraction of a Data.b value discarded`

### Requirement: A warning line that cannot be read
A warning line of either path that the runner cannot read, and a NAME
that is none of the compiler's warning names, SHALL make the cell
cannot-be-judged, naming the line.

#### Scenario: An unknown name
- **WHEN** a console line reads `x.ab3:3: warning: Something [nosuch]`
- **THEN** the cell cannot be judged and the report names that line

### Requirement: The return code of a command-line compile
In a cell of `compiler = "ab3"` or `"amiblitz3"` the query `compile.rc`
SHALL be the return code the compiler ended with (the shell's `rc=` line
behind its output), with the predicates `CMP integer` and `exists`, and no
default; a compile that did not end SHALL leave it not captured. In any
other cell an assertion, deviation or alternative end on `compile.rc`
SHALL be refused naming file, line and the compiler.

#### Scenario: A compile with a warning
- **WHEN** a cell of `compiler = "ab3"` compiles a source with one warning and writes the executable
- **THEN** `"compile.rc" = "== 5"` holds

#### Scenario: The editor path
- **WHEN** a test with only cells of `compiler = "port"` asserts `"compile.rc" = "== 0"`
- **THEN** the configuration is refused naming the file, the line and the compiler

### Requirement: No warnings query of AmiBlitz3
An assertion, deviation or alternative end on `compile.warnings` or
`compile.warnings.K` whose selector matches a cell of
`compiler = "amiblitz3"` SHALL be refused naming file, line and the
compiler.

#### Scenario: A warnings test with the oracle cell
- **WHEN** a test of `tests/ab3/model/` with the cells `ab3` and `amiblitz3` asserts `compile.warnings` in `[asserts]`
- **THEN** the configuration is refused naming the query and `amiblitz3`; with `exclude = ["compiler=amiblitz3"]` or the selector `compiler=ab3` it is accepted

## MODIFIED Requirements

### Requirement: Assertions judge what the program cannot state
An assertion SHALL be a query of an observation of the run, a predicate
(such as equal, not equal, matches, contains, exists, count) and a value.
Queries SHALL cover the exit code, requesters, the compile result, log,
warnings and return code, the printed output, the serial data, the output of `inject`, the end of
the session, the end entries, and a check's verdict, expected and actual
value. Assertions SHALL be given for all cells or for the cells a selector
of axis values matches.

#### Scenario: Exit code
- **WHEN** a cell's assertion requires exit code 0 and the program ends with 5
- **THEN** the cell fails naming the assertion, 0 and 5

#### Scenario: Printed output
- **WHEN** an assertion requires that the printed output contains "ok" and the program printed nothing
- **THEN** the cell fails naming the assertion

#### Scenario: Warnings
- **WHEN** an assertion requires `"compile.warnings" = "count == 1"` and the compile gave two warnings
- **THEN** the cell fails naming the assertion, 1 and 2

### Requirement: Cases that run on their own
In the run of all cases, a case whose assertions or deviations in a cell
of the side name an observation of the whole program (exit code,
requesters, compile result, warnings, return code or log, printed output,
serial data, inject's report, end of the session, end entry of the run)
and a case that announces a runtime error SHALL run on their own, as their
registered tests run them; in a cell with a session of its own every case
SHALL run on its own.

#### Scenario: A case that asserts the output
- **WHEN** case b of a source with the cases a, b and c asserts that the printed output contains "ok"
- **THEN** b runs on its own and the shared run holds a and c only

#### Scenario: A case that asserts the warnings
- **WHEN** case b of a source with the cases a, b and c asserts `compile.warnings`
- **THEN** b runs on its own and shares no session with other sources

#### Scenario: A test with an isolated cell
- **WHEN** all cases of a test whose cells of `libs=cd` have a session of their own run on the side `originals` with the cells `port,cd` and `orig195,cd`
- **THEN** each case runs on its own in each of the two cells, and is judged over both

### Requirement: The IDs of the jobline jobs are checked
A jobline ID SHALL match `[a-z][a-z0-9_]*`, have at most 15 characters,
be unique among the jobline elements of a cell (a `resident=` element
counts once), and SHALL NOT be `j000`, `b` followed by three digits,
`log`, `inject`, `warnings`, `rc`, or `r001` in a cell with
`editor-run = "on"`; otherwise the configuration SHALL be refused naming
file, line and ID.

#### Scenario: A jobline job named like the driver's
- **WHEN** a test sets `jobline = ["job=j000|@DRIVER@|CORPUS:out/again"]`
- **THEN** the configuration is refused naming the file, the line and `j000`

#### Scenario: Two jobline elements with one ID
- **WHEN** a test sets `jobline = ["job=j001|@DRIVER@|CORPUS:out/a", "menu=j001|9"]`
- **THEN** the configuration is refused naming `j001`

#### Scenario: A jobline job named like a query
- **WHEN** a test sets `jobline = ["job=warnings|@DRIVER@|CORPUS:out/again"]`
- **THEN** the configuration is refused naming the file, the line and `warnings`
