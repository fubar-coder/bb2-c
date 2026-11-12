# testing/bug-fix-workflow Specification

## Purpose
Defines what each fix of an original bug delivers during the bug-fix
phase, so that every fix is a documented, tested, intended deviation from
the original and the repository's rules agree on it.

## Requirements

### Requirement: What a fix delivers
A fix of an original bug SHALL come with: one issue per bug; a test of the expected behaviour (`; @fixed #NNN` or the equivalent in the suite of its area); a changed `BUG(orig)` comment ("Fixed (#N)"); an entry in `docs/*/deviations.md` as an intended deviation of kind F; the label `fixed-in-port` on the issue; and an entry on the website page of fixed original bugs. The fix commit SHALL contain a line `Fixes #NNN`, so that the issue is closed by the push.

#### Scenario: A complete fix commit
- **WHEN** a fix is committed
- **THEN** the commit contains the code change, the converted test and the deviations entry; the commit message contains `Fixes #NNN`; other steps (label, website) are done by the coordinator and named in the report

#### Scenario: No bug-preserving variant
- **WHEN** a user needs the original's behaviour
- **THEN** the documentation points to the original compiler and libraries; the project offers no switch

### Requirement: Results change only as intended
A fix SHALL change the results (executables, TED files, library files) only where the fix needs it and SHALL name the changed result in the commit; every other result SHALL stay byte-identical to the previous commit.

#### Scenario: Unrelated result differs
- **WHEN** the `prev` comparison shows a changed file the commit does not name
- **THEN** the commit is not accepted

### Requirement: Rules agree
CLAUDE.md, `.claude/agents/implementer.md`, `.claude/agents/tester.md`, `tests/README.md` and `docs/development.md` SHALL state the same fix workflow and the same test rules, and SHALL NOT say that original bugs are not fixed.

#### Scenario: Search for the old rule
- **WHEN** the files are searched for "Do not fix original bugs" or "REPRODUCED with the reference and FIXED"
- **THEN** no file states them as the current rule

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
