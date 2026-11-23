## MODIFIED Requirements

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

## ADDED Requirements

### Requirement: The driver of all cases is printed
When no case is named, the runner SHALL print on request the driver of all
cases of the source, the same text as the driver of all cases the build
step writes.

#### Scenario: The driver of all cases printed
- **WHEN** the driver is printed for a source with the cases a and b and no case is named
- **THEN** stdout holds the driver that calls the batch init, a, b and the batch end, the same text as the build step's driver of that source

### Requirement: The driver takes no parameters
The name of the result file and the selected cases SHALL be compiled into
the driver; the driver SHALL take no parameters, also in the run of all
cases.

#### Scenario: The run of all cases
- **WHEN** all cases of a source run
- **THEN** the program is the driver of all cases, started without arguments

### Requirement: All cases of a source in one run
When no case is named, the runner SHALL run all cases of the source on one
side: the driver of all cases in each cell of the side, the cells grouped
into sessions as for one case, and SHALL judge each case on its own from
its block of the result file, the batch init and the batch end, with the
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
