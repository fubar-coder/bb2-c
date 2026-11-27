---
title: rawkeylib
description: The bugs of Blitz Basic 2 in the command library rawkeylib that the
  BB2 C port fixes.
sidebar:
  order: 47
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/rawkeylib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Input

* For the raw key codes above 104, such as the reset warning and the
  extra keys of some keyboards, `BlitzKey` read a character of another
  key or a byte of the library's own code; it gives no character for
  them
  ([#245](https://github.com/fubar-coder/bb2-c/issues/245)).

::fixed-bugs-library{part="elsewhere"}
