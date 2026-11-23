# Spec Delta

## Purpose

Runs cases of several test sources of the BASIC model in shared emulator
sessions and hands each case's verdict to the case's own registered test,
so that a test is not slower than it would be alone.

## ADDED Requirements

### Requirement: Bundles are registered from the session key
The registration SHALL put the cases that may share a session with other
sources into bundles: of one registration, one session key, one set of
labels and one set of fixtures, in the order of the sources and cases, at
most 16 members each. Each bundle SHALL be one ctest test with the group's
labels and fixtures.

#### Scenario: Forty plain cases
- **WHEN** a directory registers 40 cases of the first set with the same key, labels and fixtures
- **THEN** three bundle tests are registered with 16, 16 and 8 members

#### Scenario: Seventeen plain cases
- **WHEN** a directory registers 17 such cases
- **THEN** two bundle tests are registered with 16 and 1 members

#### Scenario: Different labels
- **WHEN** two such cases differ only in the label `flaky`
- **THEN** they are in different bundles

### Requirement: A member's case test depends on its bundle
A member's case test SHALL keep its name, labels, time limit and cost,
SHALL name its bundle so that it is served from it, and SHALL depend on the
bundle only for the order: run without the bundle it SHALL run the case
alone, and a failed or skipped bundle SHALL NOT keep it from running.

#### Scenario: A selected case
- **WHEN** `ctest -R` selects one member's case test and not its bundle
- **THEN** the case runs alone

#### Scenario: A full run
- **WHEN** a preset runs a bundle and its members
- **THEN** the bundle runs first and each member's case test runs after it

### Requirement: Bundles can be switched off
A build option SHALL register no bundle when off, and a coverage build
SHALL register none; every case test then runs as without bundles.

#### Scenario: Switched off
- **WHEN** the project is configured with the bundles off
- **THEN** no bundle test is registered and no case test depends on one
