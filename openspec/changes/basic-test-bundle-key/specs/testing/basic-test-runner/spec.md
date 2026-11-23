# Spec Delta

## ADDED Requirements

### Requirement: The environment's time limit in the session key
A cell without `timeout` SHALL share an emulator session with the cells
that set `timeout` to the environment's time limit, as if it set that
value, and the registration SHALL group such cells as the runner does. A
cell without `timeout` in an environment without a time limit SHALL keep
a key of its own value "unset".

#### Scenario: Left to the environment and set to the same value
- **WHEN** the environment's time limit is 90 seconds, cell A of a test sets no `timeout` and cell B of the same test sets `timeout = 90`, both on the same machine
- **THEN** A and B run in one session

#### Scenario: Another value
- **WHEN** cell A sets no `timeout` and cell B sets `timeout = 30` in an environment with the time limit 90
- **THEN** A and B run in two sessions

### Requirement: The cases that may share a session with other sources
A case SHALL be a member of a run shared with other sources only if it is
on the side without original material, its source has exactly one cell on
that side, that cell has no session of its own (`isolate`, `serial`),
needs no test-local data and no run from the editor, does not set
`hang-after` in its configuration, its session key is
that of the default settings with the time limit of the environment, 30 or
60 seconds, and the case would not run on its own in the run of all cases.

#### Scenario: A plain case
- **WHEN** a source with one cell on the side port and the default settings has a case that asserts only check results
- **THEN** that case may share a session with other sources

#### Scenario: A case that asserts the output
- **WHEN** the same source has a case that asserts the printed output
- **THEN** that case does not share a session with other sources

#### Scenario: A test that sets hang-after
- **WHEN** a source with one port cell and otherwise the default settings sets `hang-after = 20`
- **THEN** none of its cases shares a session with other sources

#### Scenario: Two cells on the side
- **WHEN** a source has the cells `port,cd` and `port,clib`
- **THEN** none of its cases shares a session with other sources
