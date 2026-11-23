## MODIFIED Requirements

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

## ADDED Requirements

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
