## MODIFIED Requirements

### Requirement: A fix changes only the expectations of its issue
Before a fix is merged into `main`, the changed expected data SHALL be checked against the issues the fix names (`Fixes #N` lines of its commits): every added or changed case SHALL be a `[fixed #N]` case of one of these issues or the `[orig]` case paired with it. Any other added, changed or removed case SHALL be reported with its key. Such a case means that the fix also affects other behaviour; it SHALL be reviewed, and the fix commit SHALL name it with the reason (for example a further issue it fixes, with its own `Fixes #M` and `[fixed #M]` case), before the merge. Expected data are the library expected data (`libs/tests/expected`), the bug-orig tests and the corpus expected data (`tests/corpus/expect`): a bug-orig test SHALL be projected onto cases (one per section of `ID.expect` and `ID.orig`; with `; @fixed #N` the pair `[orig]` / `[fixed #N]`, without it the open test's cases), and an added `; @fixed #N` header or a changed `ID.expect` or `ID.orig` SHALL count as cases of issue N; every file of the corpus directory SHALL be projected onto the case `PASS|NAME|OBS|1` (qualifier from its name), a changed or added unqualified file being reported, and the turning of an unqualified file into `NAME.orig.EXT` and `NAME.fixed-N.EXT` of a named issue SHALL pass. The check is a C tool (`tools/results fix-scope`) run on the branch against `main`; the agent of a fix runs it and names its result in the report, the coordinator runs it again before the merge.

#### Scenario: Only the fixed issue's cases
- **WHEN** a branch with `Fixes #N` changes only `KEY [orig]` / `KEY [fixed #N]` cases
- **THEN** the check passes

#### Scenario: A fix changes another case
- **WHEN** a branch with `Fixes #N` also changes the expected value of an unqualified case or of a `[fixed #M]` case
- **THEN** the check reports that key and the merge waits until the case is reviewed and named in the commit

#### Scenario: No issue named
- **WHEN** a branch changes expected data and none of its commits has a `Fixes #N` line
- **THEN** the check reports every changed case

#### Scenario: A bug-orig fix
- **WHEN** a branch with `Fixes #7` adds `; @fixed #7` to a bug-orig test and changes only that test's `ID.expect`
- **THEN** the check passes

#### Scenario: A bug-orig fix edits another test
- **WHEN** the same branch also changes the `ID.expect` of a test with `; @fixed #8`, or of an open test
- **THEN** the check reports that test's cases

#### Scenario: A corpus fix
- **WHEN** a branch with `Fixes #6` turns `tests/corpus/expect/corpus_runerr/t124_bitmaperr.out` into `.orig.out` and `.fixed-6.out` and changes nothing else in the directory
- **THEN** the check passes

#### Scenario: A corpus fix changes another output
- **WHEN** the same branch also changes `tests/corpus/expect/corpus/t01_int.out`
- **THEN** the check reports `corpus|t01_int|out|1`
