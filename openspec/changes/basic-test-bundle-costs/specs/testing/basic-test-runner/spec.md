# Spec Delta

## ADDED Requirements

### Requirement: A served run is no run of the measured COST
For the measured COST and the statistics of the test runs, a run of a case
test served from its bundle SHALL NOT count as a run of that test, and a
test whose runs were all served SHALL keep its entry of the previous file
of measured COSTs. A bundle test SHALL get its entry from its own sessions
as any test.

#### Scenario: A served case keeps its cost
- **WHEN** a case test has the entry 2, and the runs of a full refresh served it from its bundle every time
- **THEN** the refreshed file gives it 2

#### Scenario: Served once, alone twice
- **WHEN** a case test was served in one run and ran alone with 2 and 4 seconds in two others
- **THEN** its entry is 3

#### Scenario: A bundle gets its own entry
- **WHEN** a bundle test's run has warm sessions of 3 seconds in all
- **THEN** the refreshed file gives the bundle 3
