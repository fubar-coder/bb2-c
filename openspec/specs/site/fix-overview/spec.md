# site/fix-overview Specification

## Purpose
Defines the overview page "Important Fixes & Breaking Changes" of the website: one sortable and filterable table of every fixed original bug, with its level, topic, component and fix, so that a reader with a Blitz Basic 2 program can see which fixes can change that program without reading all the pages of fixed bugs. The site build checks the table against those pages.

## Requirements

### Requirement: The overview lists the fixed original bugs
The page "Important Fixes & Breaking Changes" under `compared-with-original/` SHALL show one table row per fixed original bug, and only those: no `DIFF` case, no port bug, no item decided as designed or won't fix. A row SHALL have a level, a topic, a component, a fix and a link to the issue. The rows SHALL come from one data file with one entry per row, each with the required field `short`; the page SHALL contain no row text of its own.

#### Scenario: A row
- **WHEN** the data file has an entry for #421 with severity 1, library `polylib`, the commands `RenderToBitmap, RenderPoly` and a `short`
- **THEN** the table has a row with level 1, the topic, the component `polylib`, the two commands and the `short` sentence linking to its section, and the link to issue 421

#### Scenario: A kept bug
- **WHEN** an issue is a bug not fixed yet or a deviation of kind D
- **THEN** it has no row

### Requirement: Links keep the base path
Links of the page to other pages of the site SHALL be made with `relref`,
so that they work under the `/bb2-c/` base; the issue links point to
GitHub.

#### Scenario: Built for GitHub Pages
- **WHEN** the site is built with `--baseURL https://fubar-coder.github.io/bb2-c/`
- **THEN** every detail link starts with `/bb2-c/docs/compared-with-original/`

### Requirement: The detail pages stay
The pages of fixed original bugs SHALL be pages directly under
`compared-with-original/`: `language`, `expressions` (with its sub-page
`type-system`), `compiled-programs`, `running-blitz2`, `editor` and
`libraries` (with its groups `commands/`, `userlibs/` and `amigaos`).
There SHALL be no `fixed-bugs/` and no `new-errors/` section. They SHALL
stay the detail articles and SHALL link to the overview; a row's link
SHALL lead to the section or list section of its fix, named by the row's
`page` (relative to `compared-with-original/`) and `anchor`.

#### Scenario: Detail link
- **WHEN** a row has the page `libraries/commands/datelib` and the anchor `date-after-leap-year`
- **THEN** that heading id exists on the page `compared-with-original/libraries/commands/datelib`

### Requirement: The overview is checked against the detail pages
The site build SHALL fail, with a message that names the row or issue, when the levels are not the five levels 1 to 5 with name and text; a row lacks a required field (`short` among them); a row's page or anchor does not exist; a severity or topic is invalid or a `highlight` is not a boolean; an issue appears twice; or an issue linked on a page of fixed bugs has no row and is not on the `notRows` list with a reason.

#### Scenario: Missing anchor
- **WHEN** a row names the anchor `#no-such-section`
- **THEN** the build fails and names the row's issue

#### Scenario: Entry without row
- **WHEN** a page of fixed bugs links issue 999 in an entry and the data file has no row or exception for 999
- **THEN** the build fails and names issue 999

#### Scenario: Row without short
- **WHEN** a row has no `short`
- **THEN** the build fails and names the row's issue and the field

#### Scenario: Wrong levels list
- **WHEN** the data file lists only the levels 1 to 4
- **THEN** the build fails and names the levels it found

#### Scenario: Highlight not a boolean
- **WHEN** a row has `highlight: "yes"`
- **THEN** the build fails and names the row's issue and the value

#### Scenario: Deliberately broken input
- **WHEN** the check's test builds a site with a broken data file (missing anchor, wrong levels list, unknown severity, unknown topic, highlight not a boolean, duplicate issue, entry without row, row without `short`)
- **THEN** each case fails with its own message (the tests `site_fixoverview_*`, among them `site_fixoverview_no_short`) and the unbroken data build passes

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

### Requirement: Sorting and filtering of the table
The columns SHALL be Level, Topic, Component, Fix and Issue. With JavaScript (an own small script), the reader SHALL be able to sort by every column and filter by level and topic. Sorting SHALL be operated by keyboard-operable buttons in the headers, which carry `aria-sort`; the table SHALL use `th` with `scope`, and the filters SHALL have labels. A filter SHALL show how many rows are visible, and `?level=` and `?topic=` in the URL SHALL preselect the filters.

#### Scenario: Filter by level
- **WHEN** the reader selects level 1
- **THEN** only the level 1 rows stay visible and the count says so

#### Scenario: Sort by component
- **WHEN** the reader activates the "Component" header button twice
- **THEN** the rows are sorted by component descending and the header has `aria-sort="descending"`

#### Scenario: A filter in the URL
- **WHEN** the page is opened with `?level=3,4`
- **THEN** only the rows of levels 3 and 4 are visible and the count says so

### Requirement: The warning box lists the highlights
A row MAY have the field `highlight`, a boolean. The page SHALL show a
warning box above the levels that lists only the rows with
`highlight: true`, of any level, each with its sentence linking to its
detail section and the link to its issue, in the table's initial order.
The box SHALL end with one sentence that points to the table for all
fixes of levels 1 and 2 and gives their count.

#### Scenario: The highlights
- **WHEN** the data file has 110 rows of levels 1 and 2, eight of them with `highlight: true`
- **THEN** the warning box lists those eight rows and no other, and its last sentence names 110 fixes and links to the table

#### Scenario: A highlighted row of level 2
- **WHEN** a row of level 2 has `highlight: true`
- **THEN** it is in the warning box like a highlighted row of level 1

#### Scenario: A row of level 1 without highlight
- **WHEN** a row of level 1 has no `highlight`
- **THEN** the warning box does not list it, and its last sentence counts it with the other rows of levels 1 and 2

### Requirement: The old URLs still work
A page of fixed bugs that moved, and the removed sections `fixed-bugs/` and `new-errors/`, SHALL keep their old URLs as `aliases` in the front matter of the page that now holds their content. The template `layouts/alias.html` SHALL make the page at such a URL, forward to the page and keep the anchor of the link.

#### Scenario: An old link with an anchor
- **WHEN** a reader opens `/docs/compared-with-original/fixed-bugs/compiler/#arrays-256`
- **THEN** the page at that URL forwards to `/docs/compared-with-original/language/#arrays-256`

### Requirement: Fixes of levels 3 and 4 are entries with a program
A fix of level 3 or 4 SHALL be described on its page of fixed bugs as an
entry with its own heading (one level below the group's heading): a
program with comments for the original's and the port's results, the
reason, and "What to change". A form that only the port accepts, as a side
of such a fix, SHALL be under "Forms the original refused" on the page
"Extensions". The page "Important Fixes & Breaking Changes" SHALL link to
the table filtered to levels 3 and 4 (`?level=3,4`).

#### Scenario: A new compile error
- **WHEN** the port refuses a source the original compiled with a wrong result
- **THEN** its page of fixed bugs has an entry with the source, both results and what to change, and the row of level 4 links to it

### Requirement: Library pages and the command reference are cross-linked from the data
The links between the pages of fixed library bugs and the command
reference SHALL be made from the data file, not written by hand: the head
of a library page SHALL count its fixes by level (the rows whose `page` is
that page) and link the library in the command reference, and the end SHALL
list the rows of other pages whose `library` names this library.

#### Scenario: A fix on another page
- **WHEN** a row on page A names library B in `library`
- **THEN** the end of B's page lists that row

### Requirement: A moved entry is found by its anchor
An entry that went to another page than the alias names SHALL be listed in the front matter `anchorMoves` of the alias's page, by its anchor, and a link to that anchor SHALL go to that page. An `anchorMoves` that names a page that does not exist SHALL fail the build.

#### Scenario: A moved entry
- **WHEN** the page `language` has `anchorMoves` with `latin1-outside-string: editor` and a reader opens its alias `new-errors/names-and-types/` with the anchor `#latin1-outside-string`
- **THEN** the reader is forwarded to the page `editor` with that anchor

### Requirement: Component and fix of a row
The component of a row SHALL be derived from its `library`: `blitz2` for the compiler (also for blitz2 as a program), `ted` for the editor, the library's name for a command library or an RWE userlib, the AmigaOS library it opens (such as `exec.library`) for a library of the AmigaOS calls. The fix SHALL be the row's commands, if any, and its `short` sentence, which links to the detail section of the fix.

#### Scenario: A library of the AmigaOS calls
- **WHEN** a row's page is `libraries/amigaos` and its `library` is a deflibs library that opens `exec.library`
- **THEN** its component is `exec.library`

#### Scenario: Compiler and editor
- **WHEN** a row's `library` is `Compiler` or `TED`
- **THEN** its component is `blitz2` or `ted`
