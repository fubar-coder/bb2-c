# Spec Delta

## MODIFIED Requirements

### Requirement: Registration at build time without the emulator
A build step SHALL register the ctest tests from the sources and their
resolved configurations, without the emulator: one test per case and side,
where the side tells whether the cells need original material (the original
compiler or the CD's libraries), and the bundle tests of
`testing/basic-test-bundles`. A side without cells SHALL NOT be
registered. Labels, time limit, cost and fixtures SHALL come from the
configuration; the step SHALL depend on every configuration file it read.

#### Scenario: A changed directory configuration
- **WHEN** a `tests.toml` changes and the project is built
- **THEN** the registration of the tests below it is rebuilt

#### Scenario: A test without cells of the original
- **WHEN** a test's matrix has no cell with original material
- **THEN** only the test of the other side is registered for each case
