## Why

The overview "Important Fixes & Breaking Changes" grades every fixed
original bug on four levels. Level 1, "changes correct programs", mixes
fixes that change what a correct program computes, draws or does (the
argument order of `RenderToBitmap`, #421) with fixes a user only notices,
while the program still does its job (`Filter On` switching the filter
the other way, #985). The warning box above the table therefore lists
both kinds alike. And a new compile error ranks above a new runtime error,
although the compiler shows the one at once and the other only comes when
the program runs into it.

## What Changes

- Five severity levels instead of four (user decision of 2026-11-16).
  The level answers: can a program that ran correctly with the original
  behave differently with the port, and how hard is that to notice
  beforehand?
  1. Changes results: a correct program produces a different result that
     matters (computed values, file contents, control flow, what it draws
     or prints, or a stop).
  2. Noticeable change (new): a correct program still does its job, but
     something a user may notice differs (sound, timing, display details,
     the order or form of output) and may need adapting.
  3. New runtime error (unchanged).
  4. New compile error (the old 2).
  5. Fix without side effects (the old 4).
- The rows are renumbered (2 to 4, 4 to 5); the old level-1 rows are
  sorted into 1 or 2 one by one, and #1067 (MED tracks 8-11, measured
  against what a MED player does) goes to 2.
- The warning box shows the rows of levels 1 and 2, in two lists.
- The check fails when the data file's levels are not 1 to 5 with name
  and text; a new broken data file tests it.
- The fixed-bugs pages separate the level-1 sections ("Changes for
  existing programs") from the level-2 ones ("Changes you may notice").

## Capabilities

### Modified Capabilities
- `site/fix-overview`: five severity levels, the warning box with levels
  1 and 2, the check of the levels list.
- `testing/bug-fix-workflow`: the overview row's severity is one of the
  five levels.

## Impact

- `site/data/fixoverview.yaml`, the shortcode, the check, the stylesheet,
  the test data of the check, `site/README.md`, the pages
  `important-fixes.md`, `fixed-bugs/_index.md`, `fixed-bugs/libraries/`.
- `.claude/agents/implementer.md` (severity 1-5).
- CLAUDE.md says "severity 1-4" in fix step 3; it is changed by the
  coordinator after the user's OK.
- No effect on compiler, TED, libraries or any build result.
