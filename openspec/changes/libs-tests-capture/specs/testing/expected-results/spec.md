## MODIFIED Requirements

### Requirement: Per-check fixed values in library pair tests
A library check SHALL be able to replace "equal to the original" by a stated correct value together with the issue number, as a row of the exceptions table of the library tests (check, issue `#NNN`, the original's value, the port's value); the fixed check SHALL be asserted, not skipped. Scenarios SHALL NOT be edited to avoid a difference. The original's side of every check SHALL be the committed capture of the original (`libs/tests/captures/NAME.txt`). The test with the C libraries (`libs_test`, preset `all`, no CD needed) SHALL apply the rows; the test with the original libraries (`libs_test_originals`, preset `originals`) SHALL apply none and expect the original's value everywhere. A row SHALL hold the original's value as the committed capture has it, so that the capture stays the original's run.

#### Scenario: A fixed check
- **WHEN** a library fix changes a result and the exceptions table has a row for that check
- **THEN** `libs_test` requires the row's port value, the committed capture keeps the original's value, `libs_test_originals` still requires the original's value, and the other checks of the program still compare with the capture

#### Scenario: Replay without the CD
- **WHEN** `libs_test` runs in `all` without the CD
- **THEN** the C build's output of each test program equals its committed capture line by line, except the checks with a row, which equal the row's port value

#### Scenario: Stale recording
- **WHEN** a test program is changed and its capture is not renewed with `libs_capture`
- **THEN** `libs_test` and `libs_test_originals` fail on the first differing line

#### Scenario: Stale exemption
- **WHEN** a row's check gives the original's value in the C build, or the row's original value is not the committed capture's, or the row names a check that no longer exists
- **THEN** `libs_test` fails and names the row, so the row is removed with the fix that undoes it
