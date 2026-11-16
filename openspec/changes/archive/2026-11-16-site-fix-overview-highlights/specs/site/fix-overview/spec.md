## MODIFIED Requirements

### Requirement: The overview is checked against the detail pages
The site build SHALL fail, with a message that names the row or issue,
when the data file's levels are not the five levels 1 to 5 in order, each
with a name and a text, when a row's anchor does not exist on its page,
when a row has no valid severity or topic, when a row's `highlight` is
present and not a boolean, when an issue appears twice, or when an issue
linked in a fixed-bugs entry has no row and is not on an explicit
exception list of the data file with a reason.

#### Scenario: Missing anchor
- **WHEN** a row names the anchor `#no-such-section`
- **THEN** the build fails and names the row's issue

#### Scenario: Entry without row
- **WHEN** a fixed-bugs page links issue 999 in an entry and the data file has no row or exception for 999
- **THEN** the build fails and names issue 999

#### Scenario: Wrong levels list
- **WHEN** the data file lists only the levels 1 to 4
- **THEN** the build fails and names the levels it found

#### Scenario: Highlight not a boolean
- **WHEN** a row has `highlight: "yes"`
- **THEN** the build fails and names the row's issue and the value

#### Scenario: Deliberately broken input
- **WHEN** the check's test builds a site with a broken data file (missing anchor, wrong levels list, unknown severity, unknown topic, highlight not a boolean, duplicate issue, entry without row)
- **THEN** each case fails with its own message and the unbroken data build passes

### Requirement: Five severity levels
Every row SHALL have exactly one of five severities, which answer whether
a program that ran correctly with the original can behave differently
with the port, and how hard that is to notice beforehand: 1, changes
results; 2, noticeable change; 3, new runtime error; 4, new compile-time
error; 5, fix without side effects. The page SHALL explain the levels
above the table. The initial order SHALL be by severity, then by issue
number.

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
- **WHEN** the data file has rows of levels 1 and 2 without `highlight`
- **THEN** the warning box does not list them, and its last sentence counts them with the other rows of levels 1 and 2

## ADDED Requirements

### Requirement: The warning box shows the highlights
A row MAY have the field `highlight`, a boolean. The page SHALL show a
warning box above the levels that lists only the rows with
`highlight: true`, of any level, each with its sentence linking to its
detail section and the link to its issue, in the table's initial order.
The box SHALL end with one sentence that points to the table for all
fixes of levels 1 and 2 and gives their count.

#### Scenario: The highlights
- **WHEN** the data file has 21 rows of levels 1 and 2, six of them with `highlight: true`
- **THEN** the warning box lists those six rows and no other, and its last sentence names 21 fixes and links to the table

#### Scenario: A highlighted row of level 2
- **WHEN** a row of level 2 has `highlight: true`
- **THEN** it is in the warning box like a highlighted row of level 1

#### Scenario: The same set as the fixed-bugs index
- **WHEN** the fixed-bugs index page lists the fixes an ordinary program meets most easily under "Changes for existing programs"
- **THEN** exactly those fixes have `highlight: true`
