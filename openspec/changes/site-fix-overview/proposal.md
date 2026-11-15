## Why

A reader who has a program written for Blitz Basic 2 wants to know which
fixes of this port can change that program. The fixed original bugs are
spread over `site/content/docs/compared-with-original/fixed-bugs/`
(compiler 32 headings, libraries 52, editor 24, running Blitz 2 3, plus
list entries; 299 distinct issues are linked there), ordered by area, not
by what they do to a correct program. A fix that changes the behaviour of a
program that ran correctly (for example the argument order of
`RenderToBitmap`, #421) is buried between fixes of crashes. The reader has
no way to see "what do I have to check in my sources" without reading all
pages.

## What Changes

- A new page "What changes for existing programs" under
  `site/content/docs/compared-with-original/` with one sortable and
  filterable table of the fixed original bugs (only those: not `DIFF`
  cases, port bugs, as-designed or won't-fix items).
- Columns: severity, topic, library or command, one sentence with a link
  to the detail section of the existing fixed-bugs pages, the issue link.
- Four severity levels (decision of 2026-11-15): 1 changes the behaviour of
  correctly running programs (true incompatibilities, such as #421, #391,
  #985, #199); 2 new compile-time error (such as #1044); 3 new runtime
  error, only with runtime error checking (such as Too Many Tags);
  4 fixes a crash or wrong result without side effects for correct
  programs. The table is pre-sorted by severity, #421 first.
- Topics: graphics, strings, files, windows and gadgets, sound, compiler,
  editor, system (refined from the section names of the existing pages).
- The data live in one file under `site/data/`; a shortcode renders the
  table; a small own JavaScript sorts the columns and filters by severity
  and topic. Without JavaScript the page is a plain table.
- The existing fixed-bugs pages stay as the detail articles; they get a
  link to the overview.
- One-time fill with all existing entries (about 250 rows); the severity
  1 rows are confirmed by the user before the page is published.
- Fix workflow: from now on every fix adds one row to the data file in the
  coordinator's website step.
- A build check (Hugo only) that every row's anchor exists and every
  issue of the fixed-bugs pages has a row (see design.md for the limits).

## Capabilities

### New Capabilities
- `site/fix-overview`: the overview page, its data file, rendering,
  sorting and filtering, accessibility, and the consistency check against
  the detail pages.

### Modified Capabilities
- `testing/bug-fix-workflow`: the website step of a fix also adds the row
  of the overview.

## Impact

- New: `site/data/fixoverview.yaml` (name open, design.md), a shortcode
  and a partial under `site/layouts/`, a script and a stylesheet under
  `site/assets/` (or `static/`), the page
  `site/content/docs/compared-with-original/what-changes.md`.
- Changed: `site/README.md` (the data file and its fields),
  the `_index.md` files of the compared-with-original chapter and the
  fixed-bugs pages (links), `.github/workflows/pages.yml` only if the
  check test needs it.
- CLAUDE.md fix workflow step 3 gets one sentence; it is edited only with
  the user's OK (task 6.1).
- No effect on compiler, TED, libraries or any build result.
