# Spec Delta

## Purpose

Defines how Blitz test programs that judge themselves (test library,
`testing/basic-testlib`) are configured, registered, run through a
generated driver and judged: the hierarchical run configuration, the
matrix of cells, the assertions, and the verdicts of the runner.

## ADDED Requirements

### Requirement: Tests of the model are found by their file name
A test source of this model SHALL be named `ID.test.bas`; its own
configuration, if it has one, SHALL be `ID.test.toml` next to it, and a
directory's configuration `tests.toml`. CMake SHALL read nothing from these
files at configure time; sources with other names SHALL keep their current
form.

#### Scenario: A new test
- **WHEN** a file `foo.test.bas` is added below `tests/bug-orig/libs/` and the project is built
- **THEN** its tests are registered without any change to a CMake file

#### Scenario: An old test
- **WHEN** a bug-orig test `bar.bas` with header directives exists
- **THEN** it is registered and judged as before

### Requirement: The run configuration is hierarchical
The run configuration of a test SHALL be the merge of the `tests.toml`
files from its directory upwards up to and including the first one with
`root = true`, and of its own file. The nearest file SHALL win per key;
tables SHALL merge per key; a list value SHALL replace, never extend.

#### Scenario: A directory narrows a setting
- **WHEN** the root file sets a list setting to two values and a subdirectory's file sets it to one
- **THEN** the tests of the subdirectory have the one value

#### Scenario: No root
- **WHEN** no file with `root = true` is found up to the repository's top
- **THEN** the runner refuses the test, naming it

### Requirement: Keys of a single test are not inherited
Keys that describe one test (the fixed issue, the declared deviations of
the original and their validators) SHALL be valid only in a test's own
file; in a directory file they SHALL be an error naming file and key.

#### Scenario: A fixed issue in a directory file
- **WHEN** a `tests.toml` sets the fixed issue
- **THEN** the runner fails naming the file and the key

### Requirement: Unknown keys are errors
A key the runner does not know SHALL be an error naming the file, the line
and the key.

#### Scenario: A misspelled setting
- **WHEN** a configuration file holds `timout = 60`
- **THEN** the runner fails naming the file, the line and `timout`

### Requirement: The run settings are in the configuration
Everything the header directives describe today (configurations of
compiler and libraries, user libraries, machine, runtime error checking,
time limits, files, wrappers of the run, the cost of a test, fixtures)
SHALL be a run setting of the configuration; the runner SHALL take no run
setting from the source.

#### Scenario: A test with C libraries only
- **WHEN** a test's configuration sets the libraries to the C libraries only
- **THEN** the runner runs no cell with the CD's libraries for it

### Requirement: Paths and placeholders in the configuration
A relative path in a configuration file SHALL be relative to that file.
Placeholders for locations only the build knows (test data, user
libraries, the CD) SHALL be resolved by the runner from the build's
environment, and an unknown placeholder SHALL be an error.

#### Scenario: A file of a directory's configuration
- **WHEN** `tests/bug-orig/libs/tests.toml` names `data/x.iff` and a test two directories below inherits it
- **THEN** the runner uses `tests/bug-orig/libs/data/x.iff`

### Requirement: The effective configuration can be printed
The runner SHALL print the effective configuration of a test on request:
every key with its value and the file and line it comes from, the axes of
the matrix and the list of cells.

#### Scenario: Origin of a key
- **WHEN** the effective configuration of a test is printed whose time limit comes from its directory's file
- **THEN** the time limit is shown with that file and line

### Requirement: Any run setting can be an axis
A configuration SHALL name its axes explicitly in a matrix table: each axis
is a run setting with a list of values. A list value outside the matrix
table SHALL NOT be an axis. Keys that are not run settings (fixed issue,
assertions) SHALL NOT be axes.

#### Scenario: Runtime error checking as an axis
- **WHEN** a test's matrix makes runtime error checking an axis with off and on
- **THEN** the test runs in cells with each value, and one source serves both

#### Scenario: Assertions as an axis
- **WHEN** a matrix table names the assertions
- **THEN** the runner fails naming the key

### Requirement: Cells are the product of the axes
The cells of a test SHALL be the product of the values of its axes, minus
the combinations a configuration excludes. A cell SHALL be named by the
values of its axes only.

#### Scenario: An excluded combination
- **WHEN** the axes are compiler (port, orig) and libraries (cd, clib) and the combination orig with clib is excluded
- **THEN** the test has three cells

### Requirement: The runner generates the driver
For each run the runner SHALL generate a driver program that defines the
library's macros, holds the preprocessed text of the test source, jumps over
it, starts the library with the name of the result file, runs the batch init
only if a case is selected, calls the selected cases by name, and runs the
batch end. The runner SHALL print the driver on request.

#### Scenario: A renamed case
- **WHEN** a case is renamed in the source and the registration was not rebuilt
- **THEN** compiling the driver fails naming the case

#### Scenario: One case selected
- **WHEN** the test of case b of a source with the cases a and b runs
- **THEN** the batch init, case b and the batch end run, case a does not

#### Scenario: A compile error in the test's text
- **WHEN** the driver fails to compile because of line 12 of `ID.test.bas`
- **THEN** the runner reports `ID.test.bas:12`

### Requirement: The preprocessor numbers the checks
The runner SHALL give every check of a test source a number in source order
and put it into the check's call, and SHALL keep for every check its
namespace, its number in the namespace, name, source line and the text of
its checked expression, outside the program. The report of a failed check SHALL show its name or
number, its source line, the expression text, and the expected and actual
values.

#### Scenario: Report of a failed check
- **WHEN** the check `!CheckEq{ShapeWidth(99),16,"width"}` in line 42 of case `frames_maximum` gets 0
- **THEN** the report shows `frames_maximum`, `width`, `ID.test.bas:42`, `ShapeWidth(99)`, 16 and 0

#### Scenario: An expression with quotes
- **WHEN** a check compares `Len("abc")` with 3 and fails
- **THEN** the report shows `Len("abc")` as written and the program holds no copy of that text

### Requirement: Placeholders in a test source
The preprocessor SHALL replace a fixed set of placeholders in a test source
(for example the source line, the case, the test id, the locations of test
data) and SHALL refuse an unknown placeholder naming file and line.

#### Scenario: An unknown placeholder
- **WHEN** a test source holds `@FOO@`
- **THEN** the runner fails naming the file, the line and `@FOO@`

### Requirement: Assertions judge what the program cannot state
An assertion SHALL be a query of an observation of the run, a predicate
(such as equal, not equal, matches, contains, exists, count) and a value.
Queries SHALL cover the exit code, requesters, the compile result and log,
the printed output, the serial data, the output of `inject`, the end of
the session, the end entries, and a check's verdict, expected and actual
value. Assertions SHALL be given for all cells or for the cells a selector
of axis values matches.

#### Scenario: Exit code
- **WHEN** a cell's assertion requires exit code 0 and the program ends with 5
- **THEN** the cell fails naming the assertion, 0 and 5

#### Scenario: Printed output
- **WHEN** an assertion requires that the printed output contains "ok" and the program printed nothing
- **THEN** the cell fails naming the assertion

### Requirement: A check with several entries
A check SHALL have the verdict fail if any of its entries failed. A query of
its expected or actual value SHALL refer to its first failed entry, or to
its first entry if none failed.

#### Scenario: A check in a loop fails once
- **WHEN** a check in a loop passes twice and fails in the third run with actual value 7
- **THEN** the check's verdict is fail and its actual value is 7

### Requirement: A more specific assertion replaces
For one query, the assertion of the nearest file and of the most specific
cell selector SHALL apply alone; assertions SHALL NOT be added up.

#### Scenario: A test expects another exit code
- **WHEN** the root file asserts exit code 0 and a test's own file asserts exit code 20
- **THEN** the test's cells are judged against 20 only

### Requirement: The default assertions
Unless the configuration says otherwise, a case SHALL pass only if every
check passed, no precondition failed, its end entry and the end entry of
the run are present, the exit code is 0, and no requester was shown other
than an announced runtime error.

#### Scenario: A missing end entry of the case
- **WHEN** a case ends the program with `End` after a passing check
- **THEN** the case fails as incomplete

### Requirement: An announced runtime error is matched
When the result file announces a runtime error, the case SHALL pass this
point only if the run showed that error's requester after the announcement.

#### Scenario: The error does not come
- **WHEN** a case announces "Shape too Big" and the run shows no requester
- **THEN** the case fails naming the announcement

### Requirement: Registration at build time without the emulator
A build step SHALL register the ctest tests from the sources and their
resolved configurations, without the emulator: one test per case and side,
where the side tells whether the cells need original material (the original
compiler or the CD's libraries). A side without cells SHALL NOT be
registered. Labels, time limit, cost and fixtures SHALL come from the
configuration; the step SHALL depend on every configuration file it read.

#### Scenario: A changed directory configuration
- **WHEN** a `tests.toml` changes and the project is built
- **THEN** the registration of the tests below it is rebuilt

#### Scenario: A test without cells of the original
- **WHEN** a test's matrix has no cell with original material
- **THEN** only the test of the other side is registered for each case

### Requirement: Names of the registered tests
A registered test SHALL be named `PREFIX_ID__CASE`, and `PREFIX_ID__CASE_originals`
for the side with original material, using only lower-case letters, digits
and underscores (`-` of the id becomes `_`); a name SHALL depend on the
file and the case only. Two tests with the same name SHALL fail the
registration.

#### Scenario: An added axis
- **WHEN** a machine axis is added to a test's matrix
- **THEN** the names of its registered tests do not change

### Requirement: The structure of a test source is checked at build time
The registration step SHALL check the form of every test source against
`testing/basic-testlib` (the blocks paired in source order and in their
order, no conditional compilation cutting a block, only a comment after a
block's opening or closing macro on its line, no executable code outside
the blocks, the case names, no check inside a macro definition of the
test) and SHALL fail naming file, line and case.

#### Scenario: A case crossing a loop
- **WHEN** a case starts inside a `For` loop and ends after its `Next`
- **THEN** the build fails naming the file and the line of the case

#### Scenario: A missing end of a case
- **WHEN** the second of three cases has no end macro
- **THEN** the build fails naming the file, the line and the second case

#### Scenario: A statement after the case macro
- **WHEN** a line reads `!TestCase{x}:y=5`
- **THEN** the build fails naming the file and the line

#### Scenario: An extra end of a case
- **WHEN** a source has an end macro without a case before it
- **THEN** the build fails naming the file and the line

### Requirement: The runner groups cells
The runner SHALL decide which cells of a run share an emulator session,
from the settings that define a session; CMake SHALL NOT know the cells.

#### Scenario: Two cells, one machine
- **WHEN** a test of a case has a cell with the port and the CD's libraries and one with the C libraries on the same machine
- **THEN** both run in one session

### Requirement: Configuration files select their tests
For the selection of tests by change (`testing/test-selection`), a changed
configuration file SHALL select every test whose configuration reads it,
and a changed test source SHALL select the tests of its cases.

#### Scenario: A directory file changes
- **WHEN** `tests/bug-orig/libs/tests.toml` changes
- **THEN** every test of the model below `tests/bug-orig/libs/` is selected

### Requirement: The checks of the runner have tests with broken input
Every check of this model (unknown key, missing root, an inherited key of a
single test, a matrix key that is no run setting, an unknown placeholder, a
failing assertion, each rule of the source form, a duplicate test name, a
check call the preprocessor must number) SHALL have a host test with
deliberately broken input.

#### Scenario: Broken configuration
- **WHEN** the host test feeds a configuration with an unknown key
- **THEN** the runner's logic fails naming the key
