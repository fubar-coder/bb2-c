---
title: utillib
description: The bugs of Blitz Basic 2 in the command library utillib that the
  BB2 C port fixes.
sidebar:
  order: 60
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/utillib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Tasks and message ports

* `TaskName$` of a task without a name copied the text at address 0; it
  is the empty string
  ([#297](https://github.com/fubar-coder/bb2-c/issues/297)).
* `DeleteMsgPort` freed a port with the messages still waiting at it, so
  their senders never got them back; they are replied first
  ([#299](https://github.com/fubar-coder/bb2-c/issues/299)).
* `TaskName$` and `Wait` changed the register that holds the base of the
  library a command calls, so a later function of the same line, such as
  the second `SystemDate` in `a=SystemDate+Len(TaskName$(0))+SystemDate`,
  was called with the wrong base and could crash the program; they keep
  the register
  ([#1060](https://github.com/fubar-coder/bb2-c/issues/1060)).

#### `CreateMsgPort` without a free signal {#createmsgport-signal}

**What happened.** A task has a limited number of signals. When none was
free, the original's `CreateMsgPort` did not notice and made a port whose
messages never wake the program.

**With the port.** This is the runtime error "Failed to allocate signal
for Port", also without runtime error checking, like the error "Failed to
allocate memory for Port" of the same command.

**What to change.** Free the signals or ports the program no longer
needs before it makes a new port.

Issue: [CreateMsgPort did not notice a failed signal allocation (#298)](https://github.com/fubar-coder/bb2-c/issues/298).

#### `DeleteMsgPort` of a port `CreateMsgPort` did not make {#deletemsgport-foreign}

**What happened.** `DeleteMsgPort` took any address as a port. A port
that `CreateMsgPort` had not made, such as one the program had set up
itself, was taken apart and freed as if it had been, which corrupts
memory. With runtime error checking, only a memory error came, after the
damage.

**With the port.** With runtime error checking, such a port is the error
"Port Not Created By CreateMsgPort", before anything is changed.

**What to change.** Give `DeleteMsgPort` only ports that `CreateMsgPort`
returned, and remove other ports the way they were made.

Issue: [DeleteMsgPort took any address as a port (#1041)](https://github.com/fubar-coder/bb2-c/issues/1041).

::fixed-bugs-library{part="elsewhere"}
