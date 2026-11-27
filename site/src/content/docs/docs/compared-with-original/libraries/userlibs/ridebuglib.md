---
title: ridebuglib
description: The bugs of Blitz Basic 2 in the RWE userlib ridebuglib that the
  BB2 C port fixes.
sidebar:
  order: 16
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/ridebuglib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ridebuglib {#ridebuglib-fixes}

These fixes concern
[ridebuglib](/docs/libraries/userlibs/ridebuglib/),
the commands by Leading Edge Software that send instructions to the
runtime debugger. They act only in a program compiled with runtime error
checking.

* `AddVarTrace` and `DelVarTrace` cut a name to the 251 characters that
  the message to the debugger holds. The original copied a longer name
  over the library's own code, and the next of its commands crashed
  ([#613](https://github.com/fubar-coder/bb2-c/issues/613)).
* `AddVarTrace` and `DelVarTrace` with an empty name send just the empty
  name. The original also copied the bytes behind it in memory into the
  message, up to the next 0 byte
  ([#612](https://github.com/fubar-coder/bb2-c/issues/612)).
* `DelVarTrace` puts the name where the debugger reads it. The original
  put it four bytes further on, so the debugger read four bytes left
  from the previous message as the start of the name and deleted the
  wrong trace or none
  ([#610](https://github.com/fubar-coder/bb2-c/issues/610)).

::fixed-bugs-library{part="elsewhere"}
