---
title: utillib
weight: 60
description: The bugs of Blitz Basic 2 in the command library utillib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/utillib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Tasks and message ports

* `TaskName$` of a task without a name copied the text at address 0; it
  is the empty string
  ([#297](https://github.com/fubar-coder/bb2-c/issues/297)).
* `CreateMsgPort` did not notice when the program had no free signal
  left and made a port whose messages never wake the program; this is
  the runtime error "Failed to allocate signal for Port"
  ([#298](https://github.com/fubar-coder/bb2-c/issues/298)).
* `DeleteMsgPort` freed a port with the messages still waiting at it, so
  their senders never got them back; they are replied first
  ([#299](https://github.com/fubar-coder/bb2-c/issues/299)).
* `DeleteMsgPort` took any address as a port, so a port that
  `CreateMsgPort` had not made was freed as if it had, which corrupts
  memory, and runtime error checking reported only a memory error after
  the damage; with runtime error checking such a port is the error "Port
  Not Created By CreateMsgPort", before anything is changed
  ([#1041](https://github.com/fubar-coder/bb2-c/issues/1041)).
* `TaskName$` and `Wait` changed the register that holds the base of the
  library a command calls, so a later function of the same line, such as
  the second `SystemDate` in `a=SystemDate+Len(TaskName$(0))+SystemDate`,
  was called with the wrong base and could crash the program; they keep
  the register
  ([#1060](https://github.com/fubar-coder/bb2-c/issues/1060)).

{{% fixed-bugs-library part="elsewhere" %}}
