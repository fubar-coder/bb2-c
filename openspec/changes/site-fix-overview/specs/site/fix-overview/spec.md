
## ADDED Requirements

### Requirement: The overview lists the fixed original bugs
The page "Important Fixes & Breaking Changes" under
`compared-with-original/` SHALL show one table row per fixed original bug,
and only those: no `DIFF` case, no port bug, no item decided as designed
or won't fix. A row SHALL have a severity, a topic, a library or command,
one sentence that links to the detail section on a fixed-bugs page, and a
link to the issue. The rows SHALL come from one data file with one entry
per row; the page SHALL contain no row text of its own.

#### Scenario: A row
- **WHEN** the data file has an entry for #421 with severity 1
- **THEN** the table has a row with severity 1, the topic, `RenderToBitmap`, the sentence linking to its section, and the link to issue 421

#### Scenario: A kept bug
- **WHEN** an issue is a bug not fixed yet or a deviation of kind D
- **THEN** it has no row

### Requirement: Four severity levels
Every row SHALL have exactly one of four severities: 1, changes the
behaviour of correctly running programs; 2, new compile-time error; 3,
new runtime error, only with runtime error checking; 4, fixes a crash or
wrong result without side effects for correct programs. The page SHALL
explain the four levels above the table. The initial order SHALL be by
severity ascending, within a severity by topic, with #421 at the very top.

#### Scenario: Order without JavaScript
- **WHEN** the page is shown with JavaScript off
- **THEN** the rows are in the initial order and nothing is hidden

### Requirement: Sorting and filtering
With JavaScript, the reader SHALL be able to sort by every column and
filter by severity and by topic; the script SHALL be an own small script
without an external library. Sorting SHALL be operated by buttons in the
column headers, which are keyboard-operable and carry `aria-sort`; the
table SHALL use `th` headers with `scope`, and the filter controls SHALL
have labels. A filter SHALL show how many rows are visible.

#### Scenario: Filter by severity
- **WHEN** the reader selects severity 1
- **THEN** only the severity 1 rows stay visible and the count says so

#### Scenario: Sort by library
- **WHEN** the reader activates the "Library" header button twice
- **THEN** the rows are sorted by library descending and the header has `aria-sort="descending"`

### Requirement: Links keep the base path
Links of the page to other pages of the site SHALL be made with `relref`,
so that they work under the `/bb2-c/` base; the issue links point to
GitHub.

#### Scenario: Built for GitHub Pages
- **WHEN** the site is built with `--baseURL https://fubar-coder.github.io/bb2-c/`
- **THEN** every detail link starts with `/bb2-c/docs/compared-with-original/fixed-bugs/`

### Requirement: The detail pages stay
The pages under `fixed-bugs/` SHALL stay the detail articles and SHALL
link to the overview; a row's link SHALL lead to the section or list
section of its fix.

#### Scenario: Detail link
- **WHEN** a row links to `fixed-bugs/libraries#date-after-leap-year`
- **THEN** that heading id exists on the page

### Requirement: The overview is checked against the detail pages
The site build SHALL fail, with a message that names the row or issue,
when a row's anchor does not exist on its page, when a row has no valid
severity or topic, when an issue appears twice, or when an issue linked
in a fixed-bugs entry has no row and is not on an explicit exception
list of the data file with a reason.

#### Scenario: Missing anchor
- **WHEN** a row names the anchor `#no-such-section`
- **THEN** the build fails and names the row's issue

#### Scenario: Entry without row
- **WHEN** a fixed-bugs page links issue 999 in an entry and the data file has no row or exception for 999
- **THEN** the build fails and names issue 999

#### Scenario: Deliberately broken input
- **WHEN** the check's test builds a site with a broken data file (missing anchor, unknown severity, duplicate issue, entry without row)
- **THEN** each case fails with its own message and the unbroken data build passes
