# Spec Delta

## ADDED Requirements

### Requirement: fix-scope covers the tests that judge themselves
fix-scope SHALL check the bug-orig tests of the model of
`testing/basic-test-runner` against the issues a fix names: setting
`fixed-orig = N` or `fixed = N` in a test's own configuration SHALL pass
for issue N. A changed check or expected value in a test source, a changed
assertion or a changed declaration of a deviation SHALL be reported with
the test, the case and the check or query, and SHALL be reviewed and named
in the commit before the merge.

#### Scenario: Only the fixed issue
- **WHEN** a branch with `Fixes #N` sets `fixed-orig = N` in one test's configuration and changes nothing else of the tests
- **THEN** the check passes

#### Scenario: A fix changes an expected value
- **WHEN** the same branch also changes the expected value of a check in another test's source
- **THEN** the check reports that test, case and check
