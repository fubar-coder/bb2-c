# Spec Delta

## ADDED Requirements

### Requirement: fix-scope covers the tests that judge themselves
fix-scope SHALL check the bug-orig tests of the model of
`testing/basic-test-runner` against the issues a fix names: setting
`fixed-orig = N`, `fixed = N` or `extension = N` in a test's own
configuration SHALL pass for issue N. A changed check or expected value in a test source, a changed
assertion or a changed declaration of a deviation SHALL be reported with
the test, the case and the check or query, and SHALL be reviewed and named
in the commit before the merge.

#### Scenario: Only the fixed issue
- **WHEN** a branch with `Fixes #N` sets `fixed-orig = N` in one test's configuration and changes nothing else of the tests
- **THEN** the check passes

#### Scenario: A fix changes an expected value
- **WHEN** the same branch also changes the expected value of a check in another test's source
- **THEN** the check reports that test, case and check

#### Scenario: The test of an extension
- **WHEN** a branch with `Fixes #N` sets `extension = N` in one test's configuration and changes nothing else of the tests
- **THEN** the check passes

## MODIFIED Requirements

### Requirement: What a fix delivers
A fix of an original bug SHALL come with: one issue per bug; a test of the expected behaviour (`; @fixed-orig #NNN` or the equivalent in the suite of its area); a changed `BUG(orig)` comment ("Fixed (#N)"); an entry in `docs/*/deviations.md` as an intended deviation of kind F; the label `fixed-in-port` on the issue; an entry on the website page of fixed original bugs; and one row for the issue in the data file of the overview page (`site/data/`, with severity, one of the five levels 1-5 of `site/fix-overview`, topic, library or command, sentence, anchor and issue). The fix commit SHALL contain a line `Fixes #NNN`, so that the issue is closed by the push. The website entry and the row are the coordinator's step; the agent proposes severity and topic in its report.

#### Scenario: A complete fix commit
- **WHEN** a fix is committed
- **THEN** the commit contains the code change, the converted test and the deviations entry; the commit message contains `Fixes #NNN`; other steps (label, website entry and overview row) are done by the coordinator and named in the report

#### Scenario: No bug-preserving variant
- **WHEN** a user needs the original's behaviour
- **THEN** the documentation points to the original compiler and libraries; the project offers no switch

#### Scenario: A fix without a row
- **WHEN** the website entry of a fix is added and the data file gets no row for its issue
- **THEN** the site build fails (`site/fix-overview`, consistency check)

### Requirement: A fix changes only the expectations of its issue
Before a fix is merged into `main`, the changed expected data SHALL be checked against the issues the fix names (`Fixes #N` lines of its commits): every added or changed case SHALL be a `[fixed #N]` case of one of these issues, the `[orig]` case paired with it, or an `[orig #N]` case of one of these issues (in the library expected data: a case of the original that the port no longer reaches because of the fix of issue N, so that no `[fixed #N]` counterpart is possible); an `[orig]` case that becomes `[orig #N]` with its value SHALL count as unchanged. Any other added, changed or removed case SHALL be reported with its key. Such a case means that the fix also affects other behaviour; it SHALL be reviewed, and the fix commit SHALL name it with the reason (for example a further issue it fixes, with its own `Fixes #M` and `[fixed #M]` case), before the merge. Expected data are the library expected data (`libs/tests/expected`), the bug-orig tests and the corpus expected data (`tests/corpus/expect`): a bug-orig test SHALL be projected onto cases (one per section of `ID.expect` and `ID.orig`; with `; @fixed-orig #N` the pair `[orig]` / `[fixed #N]`, without it the open test's cases; the marker's old name `; @fixed #N`, in a base tree that still has it, SHALL give the same cases), and an added `; @fixed-orig #N` header or a changed `ID.expect` or `ID.orig` SHALL count as cases of issue N; every file of the corpus directory SHALL be projected onto the case `PASS|NAME|OBS|1` (qualifier from its name), a changed or added unqualified file being reported, and the turning of an unqualified file into `NAME.orig.EXT` and `NAME.fixed-N.EXT` of a named issue SHALL pass. The check is a C tool (`tools/results fix-scope`) run on the branch against `main`; the agent of a fix runs it and names its result in the report, the coordinator runs it again before the merge.

#### Scenario: Only the fixed issue's cases
- **WHEN** a branch with `Fixes #N` changes only `KEY [orig]` / `KEY [fixed #N]` cases
- **THEN** the check passes

#### Scenario: A case of the original only
- **WHEN** a branch with `Fixes #N` turns an unqualified library case that the fixed port no longer produces into `KEY [orig #N]`
- **THEN** the check passes; with only other issues named it reports the case

#### Scenario: A fix changes another case
- **WHEN** a branch with `Fixes #N` also changes the expected value of an unqualified case or of a `[fixed #M]` case
- **THEN** the check reports that key and the merge waits until the case is reviewed and named in the commit

#### Scenario: No issue named
- **WHEN** a branch changes expected data and none of its commits has a `Fixes #N` line
- **THEN** the check reports every changed case

#### Scenario: A bug-orig fix
- **WHEN** a branch with `Fixes #7` adds `; @fixed-orig #7` to a bug-orig test and changes only that test's `ID.expect`
- **THEN** the check passes

#### Scenario: A bug-orig fix edits another test
- **WHEN** the same branch also changes the `ID.expect` of a test with `; @fixed-orig #8`, or of an open test
- **THEN** the check reports that test's cases

#### Scenario: The old name of the header
- **WHEN** a branch turns the header `; @fixed #N` of a fixed bug-orig test into `; @fixed-orig #N` and changes nothing else of it
- **THEN** the check reports none of its cases

#### Scenario: A corpus fix
- **WHEN** a branch with `Fixes #6` turns `tests/corpus/expect/corpus_runerr/t124_bitmaperr.out` into `.orig.out` and `.fixed-6.out` and changes nothing else in the directory
- **THEN** the check passes

#### Scenario: A corpus fix changes another output
- **WHEN** the same branch also changes `tests/corpus/expect/corpus/t01_int.out`
- **THEN** the check reports `corpus|t01_int|out|1`
