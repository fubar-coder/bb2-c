---
title: crunchlib
description: The bugs of Blitz Basic 2 in the command library crunchlib that the
  BB2 C port fixes.
sidebar:
  order: 15
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/crunchlib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Decrunchers

`CrMDecrunch` and `PPDecrunch` unpack data packed with Crunch-Mania and
PowerPacker. They trust the data they are given; these fixes stop the
cases in which a wrong value in the data made them run away.

* `CrMDecrunch` in place, or with buffers that overlap, of data whose
  header gave a packed length below 4 overwrote the whole memory; it
  copies the few bytes there are
  ([#115](https://github.com/fubar-coder/bb2-c/issues/115)).
* `CrMDecrunch` of Crunch-Mania data with corrupt tables (CrM2) wrote
  behind its tables, into the library; it stops unpacking at such a
  table
  ([#117](https://github.com/fubar-coder/bb2-c/issues/117)).
* `PPDecrunch` of PowerPacker data with an efficiency value of 0 or
  above 15 in the header unpacked garbage around the destination; it
  unpacks nothing, as for data that are not PowerPacker data
  ([#118](https://github.com/fubar-coder/bb2-c/issues/118)).

::fixed-bugs-library{part="elsewhere"}
