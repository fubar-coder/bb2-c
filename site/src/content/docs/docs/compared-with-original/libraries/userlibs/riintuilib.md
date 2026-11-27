---
title: riintuilib
description: The bugs of Blitz Basic 2 in the RWE userlib riintuilib that the
  BB2 C port fixes.
sidebar:
  order: 27
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/riintuilib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in riintuilib {#riintuilib-fixes}

These fixes concern
[riintuilib](/docs/libraries/userlibs/riintuilib/), the
text output on a bitmap by Reflective Images: `IPOutput` makes a `Rast`
object the output channel, so that `Print` and `NPrint` draw on its
bitmap.

* Text printed through `IPOutput` starts at the position that `IPLocate`
  set, and the next text goes on after it. The original drew every text
  at the top left corner of the Rast, over the text before
  ([#697](https://github.com/fubar-coder/bb2-c/issues/697)).
* `NPrint` through `IPOutput` ends the line: the next text starts at the
  left edge of the next line. The original drew a byte from low memory
  as a character instead
  ([#699](https://github.com/fubar-coder/bb2-c/issues/699)).
* A text of 1024 characters or more is drawn whole. The original copied
  it into a buffer of 1024 bytes without a limit, so it wrote over the
  pens of the next text and, from 1028 characters on, over the library
  ([#698](https://github.com/fubar-coder/bb2-c/issues/698)).
* `IPColour -1,bgpen`, which leaves the foreground pen as it is, sets
  the background pen of the Rast in use. The original left the Rast's
  background pen unchanged and wrote the pen into memory of the program
  instead ([#696](https://github.com/fubar-coder/bb2-c/issues/696)).

::fixed-bugs-library{part="elsewhere"}
