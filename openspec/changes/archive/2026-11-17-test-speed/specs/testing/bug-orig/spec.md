## ADDED Requirements

### Requirement: A hanging program ends its session early in the normal run
In the normal run of a bug-orig test (port and clib sessions) a session whose first program run has started and whose job has changed no file for `BUG_ORIG_RUN_HANG_AFTER` seconds SHALL end like a session that reached its time limit (state `timeout`, same observation). The silence SHALL count from the first program run, never from the start of the job, so a compile that is silent for long on a loaded host SHALL NOT end a session. A test that only compiles (`run=off`) SHALL have no such check. A test whose program is silent for longer SHALL set its own limit (`; @pass hang_after=N`). The verdict of every bug-orig test SHALL be the same as with the time limit alone.

#### Scenario: The program crashes into the alert
- **WHEN** the program of an open bug crashes and the machine stays in the alert
- **THEN** the session ends after the silence, not at the limit, and the verdict is the one a run to the limit gives

#### Scenario: A slow compile
- **WHEN** the compile is silent for longer than the silence limit and the program then runs normally
- **THEN** the session is not ended early

#### Scenario: A pass that cannot arm
- **WHEN** a pass file has `hang_from=run` without `hang_after=`, or with `run=off`
- **THEN** the runner refuses it

### Requirement: The TED gates are checked once per run
The gates of the TED bug-orig sessions (gate 0 and gate D) SHALL run once per ctest run, in a fixture, and not in every session. The TED bug-orig tests SHALL stay single ctest entries that require the fixture; with the fixture's mark their sessions SHALL leave out gate D and the evaluation of gate 0 (the probe program of gate 0 still runs, the memory layout of the tests depends on it), without it (the runner started by hand) they SHALL run the gates as before. A gate that fails SHALL fail the fixture's setup test, publish no mark, and ctest SHALL NOT run the tests that require it.

#### Scenario: Gates fine
- **WHEN** ctest runs several TED bug-orig tests
- **THEN** the gates run in one session of the setup test and the sessions of the tests have none

#### Scenario: A gate that fails
- **WHEN** gate D fails (a script that fails on purpose in the self test)
- **THEN** the gates pass ends with exit status 1 and publishes nothing
