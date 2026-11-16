## ADDED Requirements

### Requirement: Five severity levels
Every row SHALL have exactly one of five severities, which answer whether
a program that ran correctly with the original can behave differently
with the port, and how hard that is to notice beforehand: 1, changes
results; 2, noticeable change; 3, new runtime error; 4, new compile-time
error; 5, fix without side effects. The page SHALL explain the levels
above the table and list the rows of levels 1 and 2 in a warning box. The
initial order SHALL be by severity, then by issue number.

#### Scenario: Level 1
- **WHEN** a correct program produces a different result that matters with the fix (computed values, file contents, control flow, what it draws or prints, or a stop)
- **THEN** the row has severity 1

#### Scenario: Level 2
- **WHEN** a correct program still does its job with the fix, but something a user may notice differs (sound, timing, display details, the order or form of output)
- **THEN** the row has severity 2

#### Scenario: Levels 3 to 5
- **WHEN** a fix only turns a program that went wrong into a runtime error, a source the original compiled wrongly into a compile error, or fixes a crash or wrong result without side effects for correct programs
- **THEN** the row has severity 3, 4 or 5 respectively

#### Scenario: Order without JavaScript
- **WHEN** the page is shown with JavaScript off
- **THEN** the rows are in the initial order and nothing is hidden

#### Scenario: The warning box
- **WHEN** the data file has rows of levels 1 and 2
- **THEN** the warning box lists the level-1 rows first and the level-2 rows in a second list, and no row of levels 3 to 5

## REMOVED Requirements

### Requirement: Four severity levels
**Reason**: Replaced by "Five severity levels" (user decision of 2026-11-16): level 1 is split into results that change and noticeable changes, and a new runtime error ranks above a new compile error.
**Migration**: Rows are renumbered 2 to 4 and 4 to 5; the old level-1 rows are sorted into 1 or 2 one by one.

## MODIFIED Requirements

### Requirement: The overview is checked against the detail pages
The site build SHALL fail, with a message that names the row or issue,
when the data file's levels are not the five levels 1 to 5 in order, each
with a name and a text, when a row's anchor does not exist on its page,
when a row has no valid severity or topic, when an issue appears twice,
or when an issue linked in a fixed-bugs entry has no row and is not on an
explicit exception list of the data file with a reason.

#### Scenario: Missing anchor
- **WHEN** a row names the anchor `#no-such-section`
- **THEN** the build fails and names the row's issue

#### Scenario: Entry without row
- **WHEN** a fixed-bugs page links issue 999 in an entry and the data file has no row or exception for 999
- **THEN** the build fails and names issue 999

#### Scenario: Wrong levels list
- **WHEN** the data file lists only the levels 1 to 4
- **THEN** the build fails and names the levels it found

#### Scenario: Deliberately broken input
- **WHEN** the check's test builds a site with a broken data file (missing anchor, wrong levels list, unknown severity, unknown topic, duplicate issue, entry without row)
- **THEN** each case fails with its own message and the unbroken data build passes
