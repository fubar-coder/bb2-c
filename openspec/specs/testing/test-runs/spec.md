# testing/test-runs Specification

## Purpose

Keeps the test runs short: how many ctest jobs and emulator sessions run at
once, and which tests belong to the quick presets.

## Requirements

### Requirement: Jobs follow the CPUs, emulator sessions follow the slots
The test presets (`emulator`, `all`, `oracle`, `bug-orig`, `originals`, `coverage`) SHALL run with as many ctest jobs as the machine has CPUs, and the number of simultaneous emulator sessions SHALL be limited by the slot system alone. The slot limit SHALL default, when neither `AMIGA_EMULATION_MAX` nor the file `max` of the slot directory gives a valid one, to 60 % of the online CPUs, rounded, at least 1 and at most `AMIGA_EMULATION_SLOTS_LIMIT`. The count of CPUs SHALL be a parameter of the function that computes it, so that a host test can give it.

#### Scenario: Default on 16 CPUs
- **WHEN** no limit is configured on a machine with 16 online CPUs
- **THEN** 10 sessions may run at the same time

#### Scenario: Small and absurd counts
- **WHEN** the function gets 1 CPU, 0 or a negative count, or a count that gives more than the limit
- **THEN** it returns 1 for the first three and `AMIGA_EMULATION_SLOTS_LIMIT` for the last

#### Scenario: The configuration wins
- **WHEN** `AMIGA_EMULATION_MAX` is set, or the file `max` holds a valid number
- **THEN** that number is the limit, as before

### Requirement: The host preset holds only quick tests
The `host` preset SHALL not contain the tests of the emulation library that wait for real seconds (`amiga_emulation_supervisor`); they SHALL run in the presets that run emulator tests (`all`, `emulator`, `coverage`) with `host_emulator`.

#### Scenario: The supervisor test
- **WHEN** `ctest --preset host` lists its tests
- **THEN** `amiga_emulation_supervisor` is not among them, and `ctest --preset all` runs it
