# testing/basic-test-runner Specification

## Purpose

Defines how Blitz test programs that judge themselves (test library,
`testing/basic-testlib`) are configured, registered, run through a
generated driver and judged: the hierarchical run configuration, the
matrix of cells, the assertions, and the verdicts of the runner.

## Requirements

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

### Requirement: Any run setting with a single value can be an axis
A configuration SHALL name its axes explicitly in a matrix table: each axis
is a run setting whose value is a single value, with a list of values. A
list value outside the matrix table SHALL NOT be an axis. Keys that are not
run settings (fixed issue, assertions), run settings whose value is a list
(files, user libraries, emulator changes), the setting that gives each cell
a session of its own, and the keys of the registered test (labels, cost,
fixtures) SHALL NOT be axes.

#### Scenario: Runtime error checking as an axis
- **WHEN** a test's matrix makes runtime error checking an axis with off and on
- **THEN** the test runs in cells with each value, and one source serves both

#### Scenario: Assertions as an axis
- **WHEN** a matrix table names the assertions
- **THEN** the runner fails naming the key

#### Scenario: A list setting as an axis
- **WHEN** a matrix table names the files to stage or the labels
- **THEN** the runner fails naming the key

#### Scenario: The own session as an axis
- **WHEN** a matrix table names the setting that gives each cell a session of its own
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
batch cleanup. The runner SHALL print the driver on request.

#### Scenario: A renamed case
- **WHEN** a case is renamed in the source and the registration was not rebuilt
- **THEN** compiling the driver fails naming the case

#### Scenario: One case selected
- **WHEN** the test of case b of a source with the cases a and b runs
- **THEN** the batch init, case b and the batch cleanup run, case a does not

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

### Requirement: Queries of a string value
A query `==` or `!=` of a value of a string check SHALL compare the
length and CRC32 of the whole value, as the result file gives them
(`testing/basic-test-interface`), with those the runner computes for the
assertion's string, whether the value was cut or not. `contains` and
`matches` SHALL be allowed only on a value that is not cut; on a cut value
they SHALL be a configuration error naming the assertion. The report SHALL
show a cut value as its start with `(cut)` and its length.

#### Scenario: A cut value compared with ==
- **WHEN** an assertion requires that the actual value of a string check is a string of 400 bytes, and the result file holds the start of it with the length 400 and its CRC32
- **THEN** the assertion holds

#### Scenario: A wrong CRC32 in the result file
- **WHEN** the result file gives the actual value `ab` cut from a string of length 3 with a CRC32 other than that of the assertion's string `abc`
- **THEN** the assertion `== "abc"` does not hold, and the report shows `"ab" (cut, length 3)`

#### Scenario: contains on a cut value
- **WHEN** an assertion requires that a cut value contains "x"
- **THEN** the cell cannot be judged, and the report names the assertion and that the value was cut

### Requirement: A more specific assertion replaces
For one query, one assertion SHALL apply alone; assertions SHALL NOT be
added up. The nearest file SHALL win first, and among the assertions of
that file the one with the most specific cell selector (the most terms).
Two different assertions of the same rank for a cell SHALL be an error
naming both.

#### Scenario: A test expects another exit code
- **WHEN** the root file asserts exit code 0 and a test's own file asserts exit code 20
- **THEN** the test's cells are judged against 20 only

#### Scenario: The nearest file before the selector
- **WHEN** the root file asserts exit code 5 for the cells with the CD's libraries and a test's own file asserts exit code 20 for all cells
- **THEN** the test's cells with the CD's libraries are judged against 20

#### Scenario: Two selectors of the same rank
- **WHEN** one file asserts exit code 1 for the CD's libraries and exit code 2 for the original 1.95 compiler
- **THEN** judging the cell with both fails naming the two assertions

### Requirement: Deviations are assertions of the test's own file
The declared deviations of the original SHALL be assertions in a table
`deviations."SELECTOR"` of the test's own file, the selector naming the
cells with the original's faulty part. In those cells a deviation SHALL
replace the assertion of its query; the further keys of the table (such as
the actual value of the failing check) SHALL be its validators.

#### Scenario: A failing check with its validator
- **WHEN** a test's own file declares for the cells with the CD's libraries that check `width` of case `a` fails and its actual value is 0
- **THEN** such a cell passes only if that check fails with the actual value 0

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
point only if the run showed that error's requester after the announcement,
unless an assertion or a declared deviation names the announcement.

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

### Requirement: Time limit and source of a registered test
The time limit of a registered test SHALL be the sum of the time limits of
the cells of its side, and the test SHALL name its source by its absolute
path.

#### Scenario: Two cells on a side
- **WHEN** the side with original material of a case has two cells with a time limit of 60 seconds each
- **THEN** its registered test has a time limit of 120 seconds plus the build's margin

#### Scenario: A source given by a relative path
- **WHEN** the build step is given a test source by a path relative to its directory
- **THEN** the registered test names the source by its absolute path

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

### Requirement: Ambiguous names and hidden code are refused
The registration step SHALL refuse, naming file and line: the same check
name twice in one namespace; a procedure that holds checks and has the name
of a case or of `init` or `cleanup`; a case named `init` or `cleanup` beside a
batch init or batch cleanup; and, outside the blocks, a call of one of the
test's own macros and an `INCLUDE` or `XINCLUDE`.

#### Scenario: A check name twice
- **WHEN** case `a` holds two checks named `n`
- **THEN** the build fails naming the file and the line of the second

#### Scenario: A procedure named like a case
- **WHEN** a source defines `Statement a{}` holding a check and has a case `a`
- **THEN** the build fails naming the file and the line of the check

#### Scenario: A case named init
- **WHEN** a source with a batch init has a case `init`
- **THEN** the build fails naming the file and the line of the case

#### Scenario: An include outside the blocks
- **WHEN** a source has `INCLUDE "x.bb2"` or a call of its own macro outside the blocks
- **THEN** the build fails naming the file and the line

### Requirement: The runner groups cells
The runner SHALL decide which cells of a run share an emulator session,
from the settings that define a session; CMake SHALL NOT know the cells.
A configuration SHALL be able to give every cell of a test, or the cells
that its cell selectors match, an emulator session of its own; such a
cell SHALL share its memory and `RAM:` with no other cell's program.

#### Scenario: Two cells, one machine
- **WHEN** a test of a case has a cell with the port and the CD's libraries and one with the C libraries on the same machine
- **THEN** both run in one session

#### Scenario: Every cell in a session of its own
- **WHEN** a test whose configuration gives every cell a session of its own has three cells on the same machine
- **THEN** the case runs in three sessions, one per cell

#### Scenario: The cells a selector names
- **WHEN** a test with the cells `port,cd`, `port,clib` and `orig195,cd` on the same machine gives the cells of `libs=cd` a session of their own
- **THEN** on the side `originals` the case runs in two sessions, one per cell, and on the side `port` in one

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

### Requirement: The driver of all cases is printed
When no case is named, the runner SHALL print on request the driver of all
cases of the source, the same text as the driver of all cases the build
step writes.

#### Scenario: The driver of all cases printed
- **WHEN** the driver is printed for a source with the cases a and b and no case is named
- **THEN** stdout holds the driver that calls the batch init, a, b and the batch cleanup, the same text as the build step's driver of that source

### Requirement: The driver takes no parameters
The name of the result file and the selected cases SHALL be compiled into the driver; the driver SHALL take no control data as parameters, also in the run of all cases. Arguments of the run setting `runargs` are for the tested behaviour; the driver SHALL ignore them.

#### Scenario: The run of all cases
- **WHEN** all cases of a source run
- **THEN** the program is the driver of all cases, started without arguments

#### Scenario: A test passes arguments
- **WHEN** a test sets `runargs` to a 306-character string (`wbstartuplib-clistring-overflow`)
- **THEN** the program gets the string as its argument and runs its compiled case list as without it

### Requirement: All cases of a source in one run
When no case is named, the runner SHALL run all cases of the source on one
side: the driver of all cases in each cell of the side, the cells grouped
into sessions as for one case, and SHALL judge each case on its own from
its block of the result file, the batch init and the batch cleanup, with the
assertions and deviations of the run of that case alone. The registered
tests SHALL stay one per case and side.

#### Scenario: One case fails a check
- **WHEN** the run of all cases of a source with the cases a, b and c shows a failed check in b and every cell's program ended normally
- **THEN** a and c are judged passed from that run, and b is judged as its own run judges it

#### Scenario: ctest is unchanged
- **WHEN** the tests of a source with two cases are registered
- **THEN** there are two tests per side, each running one case

### Requirement: Cases that run on their own
In the run of all cases, a case whose assertions or deviations in a cell
of the side name an observation of the whole program (exit code,
requesters, compile result or log, printed output, serial data, inject's
report, end of the session, end entry of the run) and a case that announces
a runtime error SHALL run on their own, as their registered tests run
them; in a cell with a session of its own every case SHALL run on its own.

#### Scenario: A case that asserts the output
- **WHEN** case b of a source with the cases a, b and c asserts that the printed output contains "ok"
- **THEN** b runs on its own and the shared run holds a and c only

#### Scenario: A test with an isolated cell
- **WHEN** all cases of a test whose cells of `libs=cd` have a session of their own run on the side `originals` with the cells `port,cd` and `orig195,cd`
- **THEN** each case runs on its own in each of the two cells, and is judged over both

### Requirement: A case open at an unclean end runs again
A cell's program in a shared run ends uncleanly if its session did not
end, it did not compile, it showed a requester, its exit code is not 0 or
the run's end entry is missing. Then the case whose block was open at that
end SHALL run on its own and the cases not judged SHALL run again without
it; with no open block, each case not judged SHALL run on its own.

#### Scenario: A case ends the program
- **WHEN** case b ends the program with `End` in the run of all cases of the cases a, b and c
- **THEN** b runs on its own, a and c run again without b, and each verdict is that of the run it was judged from

#### Scenario: A crash in the batch init
- **WHEN** the program of the run of all cases crashes in the batch init
- **THEN** every case runs on its own

### Requirement: A failed case is confirmed on its own
A case judged failed, broken or error in a run shared with other cases
SHALL run again on its own; its verdict on its own SHALL count, and the
report SHALL name a case whose verdict on its own differs from its verdict
in the shared run.

#### Scenario: Memory wrecked by an earlier case
- **WHEN** case c fails in the run of all cases and passes on its own
- **THEN** c is judged passed and the report names that it failed in the run shared with the cases before it

### Requirement: The report and exit status of the run of all cases
The run of all cases SHALL report each case with the line `ID.test.bas:
case NAME: VERDICT`, marked when the case was judged on its own, and end
with a line naming the number of cases, the side and the worst verdict.
Its exit status SHALL be that of the worst verdict, in the order error
(2), broken (3), failed (4), skipped (77), reproduced (1), passed (0); it
SHALL be 77 when the side is skipped before any session (no emulator, or
a file of the side is missing).

#### Scenario: One failed, one reproduced
- **WHEN** a source's case a is reproduced and case b failed
- **THEN** the report holds a line for a and for b and the summary line, and the exit status is 4

#### Scenario: An error beats a failure
- **WHEN** one case cannot be judged and another failed
- **THEN** the exit status is 2

### Requirement: Alternative ends are declared
The test's own file SHALL be able to declare, for the cells with the
original's faulty part, alternative ends with a required reason: named
sets of assertions of the session end, the exit code, requesters, the end
entries, the printed output and checks. The declaration SHALL be a
deviation of the original. Without a reason, with an empty alternative, in
a directory's file, or with a fixed issue that is no original bug or
extension it SHALL be refused.

#### Scenario: Without a reason
- **WHEN** a test's own file declares alternative ends without a reason
- **THEN** the runner fails naming the file and line

### Requirement: A cell with alternative ends is judged
Where the deviations apply, the queries an alternative names SHALL be
judged by the alternatives instead of the other assertions and the
defaults: the cell SHALL pass that point if every assertion of one
alternative holds. The report SHALL name the alternative that held, or
each alternative with its failed assertion.

#### Scenario: An original that returns or hangs
- **WHEN** an open bug's test declares for `libs=cd` the alternatives "returns" (session done, exit code 0) and "hangs" (session timeout, no end entry of the run), and the original hangs
- **THEN** the cell passes with the alternative "hangs" and the case is reproduced, not broken or failed

#### Scenario: No alternative holds
- **WHEN** the same cell halts
- **THEN** the case fails, naming both alternatives and the session end

### Requirement: The query of inject's report
When the program of a cell ran through inject (inject's `pad`, or a program
started through inject), the query `inject` SHALL hold the lines of
inject's report (the lines starting with `inject:` of the program's
output), and the printed output SHALL keep them; otherwise `inject` SHALL
be not captured, which fails an assertion that queries it.

#### Scenario: The guard's report
- **WHEN** a cell runs its program through inject with `guard` and inject reports a write behind an allocation of 8 bytes
- **THEN** `inject` contains "written behind an allocation of 8 bytes", and so does the printed output

#### Scenario: Without inject
- **WHEN** a cell's program does not run through inject and an assertion queries `inject`
- **THEN** the cell fails naming the assertion as not captured

### Requirement: An announcement is judged like a check
A named announcement of a runtime error SHALL be the query `check.NS.NAME`
with the verdict `ok` (its requester came), `fail` (it did not come) or
`not-run` (the run did not reach it), so that an assertion or a declared
deviation can judge it; a requester that matches no announcement SHALL
count for the query `requester`.

#### Scenario: The original shows another error
- **WHEN** a case announces "Illegal Shape Depth" as `refused`, the port shows that error, and the test's own file declares for `libs=cd` that `check.CASE.refused` is `fail` and the requester contains "Can't Read File"
- **THEN** the port's cells pass with the announcement, and the cells with the CD's libraries pass with the declared deviation when the original shows "Can't Read File"

#### Scenario: The deviation removed
- **WHEN** the same test runs without the deviation
- **THEN** the cells with the CD's libraries fail naming the announcement that did not come

### Requirement: Tests of open bugs join the order of the bug-orig tests
A registered test of an open bug without a configured cost SHALL get the
sum of its cells' session limits as its cost, and SHALL be declared with
the bug-orig tests of the old form in the order of their cost, the largest
first.

#### Scenario: A test of an open bug between two of the old form
- **WHEN** a test of an open bug with one cell of a session limit of 90 seconds is registered next to bug-orig tests of the old form with the costs 240 and 10
- **THEN** its cost is 90 and it is declared after the one of 240 and before the one of 10

### Requirement: A measured COST
A checked-in file SHALL give the measured COST of tests, written by a tool
from the JUnit files of test runs: per test the median over the runs of the
sum of its emulator sessions' times without the wait for a slot, a session
that did not start from a snapshot counted as the test's median warm
session; a test without a warm session SHALL get no entry. The build SHALL
give a test with an entry that COST in place of its estimate.

#### Scenario: A cold session does not make a test dear
- **WHEN** a test's run has a warm session of 3 seconds and one that booted cold for 40 seconds, and the test's warm sessions have a median of 2 seconds
- **THEN** that run counts 5 seconds

#### Scenario: A measurement replaces the estimate
- **WHEN** a test of an open bug without `cost`, of one cell of a session limit of 60 seconds, has the entry 7
- **THEN** its COST is 7

#### Scenario: An entry of a removed test
- **WHEN** the file names a test that the build no longer has
- **THEN** the configuration succeeds without an error or warning and every test keeps its COST

### Requirement: A configured cost wins over a measurement
A test of the model whose run configuration sets `cost` SHALL keep that
COST whether the file of measured COSTs has an entry for it or not.

#### Scenario: Key and measurement
- **WHEN** a test of the model has `cost = 3` in its run configuration and the file of measured COSTs gives it 44
- **THEN** its COST is 3

### Requirement: Stale and malformed entries of the measured COSTs
An entry of the file of measured COSTs that names no test SHALL be ignored
without an error or warning, and a test without an entry SHALL keep its
estimate. A malformed line SHALL stop the configuration, naming the file's
line.

#### Scenario: A test without an entry
- **WHEN** a test of an open bug without `cost` has no entry in the file
- **THEN** its COST is its estimate

#### Scenario: A malformed line
- **WHEN** a line of the file is not "NAME COST"
- **THEN** the configuration stops naming the file's line

### Requirement: A flaky test is marked
A test's own `ID.test.toml` MAY set `flaky` to a non-empty string that
says what is known of a test that fails in full runs but passes alone.
Every registered test of such a source, on both sides, SHALL carry the
ctest label `flaky`; the key SHALL NOT change the verdict of a cell or a
case. An empty string or a value that is no string SHALL be an error
naming the file and the line; the key in a directory's `tests.toml` SHALL
be an error naming the file, the line and the key.

#### Scenario: A marked test
- **WHEN** a test's `ID.test.toml` sets `flaky = "rc 20 as the second cell of a session"`
- **THEN** its registered tests of both sides have the label `flaky` besides their other labels, and they are judged as without the key

#### Scenario: An empty description
- **WHEN** a test's `ID.test.toml` sets `flaky = ""`
- **THEN** the registration fails naming the file and the line

#### Scenario: The key in a directory file
- **WHEN** a `tests.toml` sets `flaky`
- **THEN** the registration fails naming the file, the line and the key

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

### Requirement: A program run from the editor
A run setting `editor-run` SHALL take `off` (the default) or `on`. In a
cell with `on` the driver SHALL be compiled as in any cell and then run
once with the editor's COMPILE/RUN instead of from the shell, without
arguments; its result file SHALL be judged as that of a shell run.

#### Scenario: The run of a cell from the editor
- **WHEN** a case of a cell with `editor-run = "on"` passes its checks
- **THEN** the program ran once, from the editor, and the case passes from that run's result file

#### Scenario: No second writer of the result file
- **WHEN** a cell with `editor-run = "on"` has run
- **THEN** no shell run of the driver followed in that cell

### Requirement: The observations of a run from the editor
In a cell with `editor-run = "on"` the printed output SHALL be what the
program wrote to the editor's console. The exit code and the requesters
SHALL be not captured there: their default assertions SHALL NOT apply, and
an assertion on `rc` or `requester` in such a cell SHALL be refused naming
file, line and the setting.

#### Scenario: The console is the output
- **WHEN** the program of an editor-run cell prints "start" and ends
- **THEN** the query `output` holds "start"

#### Scenario: An assertion on the exit code
- **WHEN** a test sets `editor-run = "on"` and asserts `rc = "== 0"` for the same cells
- **THEN** the configuration is refused naming the file, the line of the assertion and `editor-run`

### Requirement: Settings that need a shell run
A cell with `editor-run = "on"` SHALL be refused, naming file, line and
both keys, when it sets `runwrap`, `runargs`, `launcherargs` or `pad = "on"`;
a case of such a cell with an announced runtime error SHALL be refused
naming the source line of the announcement.

#### Scenario: A wrapper of the program
- **WHEN** a test sets `editor-run = "on"` and `runwrap = "@INJECT@|guard"`
- **THEN** the configuration is refused naming both keys

#### Scenario: An announced runtime error
- **WHEN** a case of an editor-run cell holds `!ExpectError{"Shape too Big"}`
- **THEN** the test is refused naming the file and the line of the announcement

### Requirement: The debugger module of a cell
A run setting `defaultdbug` SHALL name a file, as a path setting, that
SHALL be `blitz2:defaultdbug` in every cell the setting applies to, on
either side. Without it a cell with original material SHALL get the CD's
module and another cell none. A missing module SHALL skip the side as a
missing file does.

#### Scenario: The CD's module in a cell with the C libraries
- **WHEN** a test sets `defaultdbug = "@CD_BLITZ2@/DEFAULTDBUG"` and its cell has the C libraries
- **THEN** that cell's `blitz2:defaultdbug` is the CD's module and the cell stays on the side port

#### Scenario: Without the CD
- **WHEN** the module a cell of the side port names is missing
- **THEN** the test of that side exits 77 before any session

### Requirement: A debugger module needs a run from the editor
A cell that sets `defaultdbug` and not `editor-run = "on"` SHALL be
refused naming file, line and both keys: a module is used only by a run
from the editor and is never part of an executable.

#### Scenario: A module without an editor run
- **WHEN** a test sets `defaultdbug` and leaves `editor-run` off
- **THEN** the configuration is refused naming `defaultdbug` and `editor-run`

### Requirement: The query of inject's report of the compiler
When a cell's compiler runs through inject (the `wrap` of inject), the
query `compile.inject` SHALL hold the lines starting with `inject:` of the
compiler's output; otherwise it SHALL be not captured, which fails an
assertion that queries it. It SHALL count as an observation of the whole
program for the cases that run on their own.

#### Scenario: The tracked allocations
- **WHEN** a cell's compiler runs through inject with `track` and inject reports "inject: track: 0 allocations left, 0 bytes"
- **THEN** `compile.inject` contains that line and `compile.log` keeps it

#### Scenario: A compiler without inject
- **WHEN** a cell's compiler does not run through inject and an assertion queries `compile.inject`
- **THEN** the cell fails naming the assertion as not captured

#### Scenario: A case that asserts it runs on its own
- **WHEN** case b of a source with the cases a, b and c asserts `compile.inject`
- **THEN** in the run of all cases b runs on its own

### Requirement: The driver's source in a job line
The placeholder `@DRIVER@` in an element of `jobline` SHALL stand for the
driver's tokenised source on the cell's volume; elsewhere it SHALL be an
error naming file, line and placeholder.

#### Scenario: A second compile of the test's source
- **WHEN** a test sets `jobline = ["job=j001|@DRIVER@|CORPUS:out/again"]`
- **THEN** the cell's job list compiles the driver's source a second time as job j001

### Requirement: The prev comparison leaves out the run from the editor
The comparison with the prev build SHALL compile the driver of an
editor-run test as that of any other test and SHALL NOT run it from the
editor; the debugger module SHALL NOT be staged for it.

#### Scenario: An editor-run test with a prev build
- **WHEN** the prev comparison of a test with `editor-run = "on"` runs
- **THEN** both compilers compile the driver as job j000 and no program runs

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
