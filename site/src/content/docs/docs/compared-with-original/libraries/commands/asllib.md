---
title: asllib
description: The bugs of Blitz Basic 2 in the command library asllib that the
  BB2 C port fixes.
sidebar:
  order: 5
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/asllib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### File requesters {#file-requester-fixes}

`FileRequest$` opens the file requester of Blitz Basic 2; the editor's
file requester is the same code and has the same fixes, except for the
[lengths of the path and the file name](/docs/compared-with-original/libraries/commands/freqlib/#filerequest-lengths), which only
`FileRequest$` sets. `ASLFileRequest$`, `ASLPathRequest$` and
`ASLScreenRequest` open the requesters of the system's `asl.library`.

* `ASLFileRequest$` and `ASLPathRequest$` without `asl.library`, such as
  on Kickstart 1.3, returned a string whose length was the address of the
  title, so an assignment copied that many bytes; they return the empty
  string, as Cancel does
  ([#62](https://github.com/fubar-coder/bb2-c/issues/62)).
* `ASLFileRequest$` and `ASLPathRequest$` asked `asl.library` to free a
  requester at address 0 when the requester could not be allocated; they
  free only one that was allocated
  ([#403](https://github.com/fubar-coder/bb2-c/issues/403)).
* Every `ASLFileRequest$` and `ASLPathRequest$` kept a copy of its title
  and pattern in memory until the program ended; the copies are freed
  after each request
  ([#63](https://github.com/fubar-coder/bb2-c/issues/63)).
* `ASLPathRequest$` wrote the file name of the requester, after OK, to an
  address it had not been given: in the form with a position, the value
  of `x`, so `x=20` overwrote two exception vectors of the processor; it
  writes nothing there, and the result is the same
  ([#61](https://github.com/fubar-coder/bb2-c/issues/61)).
* `ASLPathRequest$` without a position and size, after an
  `ASLFileRequest$` with a pattern, showed a pattern gadget with that
  pattern; it shows none
  ([#64](https://github.com/fubar-coder/bb2-c/issues/64)).
* `ASLScreenRequest` freed the requester and then returned its address,
  so the program read the chosen mode from freed memory; it returns the
  address of a copy of the requester's fields, which stays valid until
  the next `ASLScreenRequest`
  ([#60](https://github.com/fubar-coder/bb2-c/issues/60)).

::fixed-bugs-library{part="elsewhere"}
