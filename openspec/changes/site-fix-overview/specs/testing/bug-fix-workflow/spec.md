## MODIFIED Requirements

### Requirement: What a fix delivers
A fix of an original bug SHALL come with: one issue per bug; a test of the expected behaviour (`; @fixed #NNN` or the equivalent in the suite of its area); a changed `BUG(orig)` comment ("Fixed (#N)"); an entry in `docs/*/deviations.md` as an intended deviation of kind F; the label `fixed-in-port` on the issue; an entry on the website page of fixed original bugs; and one row for the issue in the data file of the overview page (`site/data/`, with severity, topic, library or command, sentence, anchor and issue). The fix commit SHALL contain a line `Fixes #NNN`, so that the issue is closed by the push. The website entry and the row are the coordinator's step; the agent proposes severity and topic in its report.

#### Scenario: A complete fix commit
- **WHEN** a fix is committed
- **THEN** the commit contains the code change, the converted test and the deviations entry; the commit message contains `Fixes #NNN`; other steps (label, website entry and overview row) are done by the coordinator and named in the report

#### Scenario: No bug-preserving variant
- **WHEN** a user needs the original's behaviour
- **THEN** the documentation points to the original compiler and libraries; the project offers no switch

#### Scenario: A fix without a row
- **WHEN** the website entry of a fix is added and the data file gets no row for its issue
- **THEN** the site build fails (`site/fix-overview`, consistency check)
