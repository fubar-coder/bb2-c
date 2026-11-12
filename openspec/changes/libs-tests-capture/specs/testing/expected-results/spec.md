# Spec Delta

## REMOVED Requirements

### Requirement: Per-check fixed values in library pair tests
**Reason**: The library tests run one library set against committed expected data (`testing/library-capture`); `expect_fixed` and the pair test no longer exist, and a fixed check is a pair of cases of the shared model (`testing/test-model`).
**Migration**: See the requirement "A fixed library check is a pair of cases" below.

## ADDED Requirements

### Requirement: A fixed library check is a pair of cases
A fixed library check SHALL follow the shared test model (`testing/test-model`): it SHALL be two cases of the library test, `KEY [orig]` with the original's value (run only by `libs_emulator_originals`, preset `originals`) and `KEY [fixed #N]` with the port's value after the fix (run only by `libs_emulator`, preset `all`, no CD needed), both in the committed expected data of the program (`libs/tests/expected/NAME.txt`). The fixed case SHALL be asserted, not skipped. Scenarios SHALL NOT be edited to avoid a difference. No exception table SHALL exist; the earlier `expect_fixed` is replaced by this pair.

#### Scenario: A fixed check
- **WHEN** a library fix changes a result
- **THEN** the expected data has `KEY [orig]` with the original's value and `KEY [fixed #N]` with the port's value, `libs_emulator` requires the second, `libs_emulator_originals` the first, and the other cases of the program compare unchanged

#### Scenario: Test without the CD
- **WHEN** `libs_emulator` runs in `all` without the CD
- **THEN** the port's output of every case equals the expected data case by case, `[fixed #N]` cases included and `[orig]` cases not run

#### Scenario: Scenario changed, expected data not renewed
- **WHEN** a test program is changed and its expected data is not recorded again with `libs-capture`
- **THEN** `libs_emulator` and `libs_emulator_originals` fail on the first differing case

#### Scenario: The port regresses a fixed check
- **WHEN** the port gives the original's value for a `[fixed #N]` case
- **THEN** `libs_emulator` fails on that case
