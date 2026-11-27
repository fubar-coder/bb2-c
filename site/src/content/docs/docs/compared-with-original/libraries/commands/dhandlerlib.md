---
title: dhandlerlib
description: The bugs of Blitz Basic 2 in the command library dhandlerlib that
  the BB2 C port fixes.
sidebar:
  order: 18
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/dhandlerlib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Debugger and runtime error checking {#debugger-fixes}

One more fix of this area changes what an existing program does:
[Runtime error checking in supervisor mode outside `SetInt`](/docs/compared-with-original/libraries/commands/rundebuglib/#supervisor-mode-traps).

* RUN with the debugger started the debugger process without flushing
  the CPU caches, so on a 68020 to 68060 the process could run stale
  code; the caches are flushed before it starts
  ([#409](https://github.com/fubar-coder/bb2-c/issues/409)).

::fixed-bugs-library{part="elsewhere"}
