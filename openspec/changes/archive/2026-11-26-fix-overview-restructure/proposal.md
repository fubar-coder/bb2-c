# Proposal

## Why

The spec `site/fix-overview` still describes the site before its
restructure, which is merged on main (`f82b85a18`, `2e9d3725c`,
`26e1ed5e4`, `b23225453`): the pages of fixed bugs under `fixed-bugs/`,
the link prefix `fixed-bugs/`, a table with the columns severity, topic,
library or command and a sentence, and a check without the field `short`.

## What Changes

- MODIFIED "The overview lists the fixed original bugs": the row has a
  component and a fix (commands and `short`); the field `short` is
  required.
- REMOVED "Sorting and filtering" and ADDED "Sorting and filtering of the table" (the scenarios were named after the old columns): the columns Level, Topic, Component,
  Fix and Issue; the filters also come from the URL (`?level=`, `?topic=`).
- MODIFIED "Links keep the base path": the prefix is
  `/bb2-c/docs/compared-with-original/`.
- MODIFIED "The detail pages stay": the pages of fixed bugs are
  direct pages of the chapter, `compiler`, `running-blitz2`, `editor`
  and `libraries` instead of `fixed-bugs/`.
- MODIFIED "The overview is checked against the detail pages": the row
  without `short`, the pages checked, the test `site_fixoverview_no_short`.
- REMOVED "The warning box shows the highlights" and ADDED "The warning box lists the highlights": the scenario that
  equated the highlights with the index page of fixed bugs becomes one
  for a row without `highlight` (the index page is gone); the numbers are
  those of the data file now.
- ADDED "The old URLs still work" (aliases, `layouts/alias.html`,
  `anchorMoves`).
- ADDED "Fixes of levels 3 and 4 are entries with a program".
- ADDED "Library pages and the command reference are cross-linked from
  the data".

Spec only; the implementation is on main. The Purpose line of the main
spec is adjusted by hand (the delta format has no Purpose).
