## MODIFIED Requirements

### Requirement: Per-check fixed values in library pair tests
A library check SHALL be able to replace "equal to the original" by a stated correct value together with the issue number, as a row of the exceptions table of the library tests (check, issue `#NNN`, the original's value, the port's value); the fixed check SHALL be asserted, not skipped. Scenarios SHALL NOT be edited to avoid a difference. The original's side of every check SHALL be the committed capture of the original (`libs/tests/captures/NAME.txt`) in `libs_emulator` (preset `all`, no CD needed), and a capture run with the original in `libs_emulator_originals` (preset `originals`) SHALL give the committed capture again. A row SHALL hold the original's value as the committed capture has it, so that the capture stays the original's run.

#### Scenario: A fixed check
- **WHEN** a library fix changes a result and the exceptions table has a row for that check
- **THEN** the C build must give the row's port value, the committed capture keeps the original's value, and the other checks of the program still compare with the capture

#### Scenario: Replay without the CD
- **WHEN** `libs_emulator` runs in `all` without the CD
- **THEN** the C build's output of each test program equals its committed capture line by line, except the checks with a row, which equal the row's port value

#### Scenario: Stale recording
- **WHEN** a test program is changed and its capture is not renewed
- **THEN** `libs_emulator_originals` fails because the fresh capture of the original differs from the committed one

#### Scenario: Stale exemption
- **WHEN** a row's check gives the original's value in the C build, or the row's original value is not the committed capture's, or the row names a check that no longer exists
- **THEN** the test fails and names the row, so the row is removed with the fix that undoes it
