# Spec Delta

## ADDED Requirements

### Requirement: A fix changes only the expectations of its issue
Before a fix is merged into `main`, the changed expected data SHALL be checked against the issues the fix names (`Fixes #N` lines of its commits): every added or changed case SHALL be a `[fixed #N]` case of one of these issues or the `[orig]` case paired with it. Any other added, changed or removed case SHALL be reported with its key. Such a case means that the fix also affects other behaviour; it SHALL be reviewed, and the fix commit SHALL name it with the reason (for example a further issue it fixes, with its own `Fixes #M` and `[fixed #M]` case), before the merge. The check is a C tool (`tools/results fix-scope`) run on the branch against `main`; the agent of a fix runs it and names its result in the report, the coordinator runs it again before the merge.

#### Scenario: Only the fixed issue's cases
- **WHEN** a branch with `Fixes #N` changes only `KEY [orig]` / `KEY [fixed #N]` cases
- **THEN** the check passes

#### Scenario: A fix changes another case
- **WHEN** a branch with `Fixes #N` also changes the expected value of an unqualified case or of a `[fixed #M]` case
- **THEN** the check reports that key and the merge waits until the case is reviewed and named in the commit

#### Scenario: No issue named
- **WHEN** a branch changes expected data and none of its commits has a `Fixes #N` line
- **THEN** the check reports every changed case
