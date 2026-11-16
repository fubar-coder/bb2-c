## Why

The warning box above the table of "Important Fixes & Breaking Changes"
lists every row of levels 1 and 2, 21 entries, and is too long to be read
as a warning (user feedback of 2026-11-16). It should hold only the
highlights, the fixes an ordinary program meets most easily, such as the
operator `=<` (#9).

## What Changes

- A row of the data file may have the optional field `highlight`, a
  boolean. The warning box lists only the rows with `highlight: true`, of
  any level, each with its sentence and links as before, and ends with
  one sentence that points to the table, at levels 1 and 2, for all fixes
  that can change a correct program, with their count.
- The highlights are the six fixes of the hand-written list "Changes for
  existing programs" on the fixed-bugs index page (#9, #324, #391, #421,
  #951, #1046), so both places agree. That list stays hand-written: its
  entries say what to change in a source, which the rows' sentences do
  not.
- The check fails for a `highlight` that is not a boolean; a new broken
  data file tests it.

## Capabilities

### Modified Capabilities
- `site/fix-overview`: the warning box shows the highlighted rows; the
  check of the field `highlight`.

## Impact

- `site/data/fixoverview.yaml`, the shortcode, the check, the test data
  and test list of the check, `site/README.md`, the fixed-bugs index
  page.
- No effect on compiler, TED, libraries or any build result.
