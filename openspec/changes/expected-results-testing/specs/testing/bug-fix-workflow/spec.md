# Spec Delta

## Purpose

Defines what each fix of an original bug delivers during the bug-fix
phase, so that every fix is a documented, tested, intended deviation from
the original and the repository's rules agree on it.

## ADDED Requirements

### Requirement: What a fix delivers
A fix of an original bug SHALL come with: one issue per bug; a test of the expected behaviour (`; @fixed #NNN` or the equivalent in the suite of its area); a changed `BUG(orig)` comment ("Fixed (#N)"); an entry in `docs/*/deviations.md` as an intended deviation of kind F; the label `fixed-in-port` on the issue; and an entry on the website page of fixed original bugs. The issue SHALL be closed with a comment after the push.

#### Scenario: A complete fix commit
- **WHEN** a fix is committed
- **THEN** the commit contains the code change, the converted test and the deviations entry; other steps (label, website, closing comment) are done by the coordinator and named in the report

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
