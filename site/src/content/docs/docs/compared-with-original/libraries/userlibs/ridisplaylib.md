---
title: ridisplaylib
description: The bugs of Blitz Basic 2 in the RWE userlib ridisplaylib that the
  BB2 C port fixes.
sidebar:
  order: 18
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/ridisplaylib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ridisplaylib {#ridisplaylib-fixes}

These fixes concern
[ridisplaylib](/docs/libraries/userlibs/ridisplaylib/),
the unfinished start of a display library by Reflective Images. Its
`InitView` allocates a copper list that is never shown.

* `InitView` writes the moves of the display window and data fetch
  registers into its list. The original wrote words read from fixed
  addresses of low memory instead, and two of these addresses are odd,
  which crashed a 68000
  ([#889](https://github.com/fubar-coder/bb2-c/issues/889)).
* `InitView` sizes the list by the number of colours it is given, and
  computes the size as a long. The original used the height instead of
  the number of colours and computed a word, so large values gave a
  wrong or negative size
  ([#890](https://github.com/fubar-coder/bb2-c/issues/890)).
* Without enough chip memory, `InitView` returns 0 and writes nothing.
  The original wrote its list to address 0, over system memory, and
  freed address 0 at the end of the program
  ([#888](https://github.com/fubar-coder/bb2-c/issues/888)).
* At the end of the program the library frees the lists of all
  `InitView` calls, also when it could not open graphics.library 39.
  The original freed only the last list, and only with
  graphics.library 39
  ([#891](https://github.com/fubar-coder/bb2-c/issues/891)).

::fixed-bugs-library{part="elsewhere"}
