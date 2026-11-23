## ADDED Requirements

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
