---
title: staticslib
description: The bugs of Blitz Basic 2 in the command library staticslib that
  the BB2 C port fixes.
sidebar:
  order: 56
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/staticslib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Statements and Functions

* A Statement or Function with three or more parameters and a local
  NewType variable got a wrong third parameter, and with five parameters a
  wrong fifth one too: the library routine that sets up the local
  variables at the start of the procedure overwrote them; every parameter
  keeps its value
  ([#1063](https://github.com/fubar-coder/bb2-c/issues/1063)).

::fixed-bugs-library{part="elsewhere"}
