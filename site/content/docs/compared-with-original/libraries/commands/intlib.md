---
title: intlib
weight: 33
description: The bugs of Blitz Basic 2 in the command library intlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/intlib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Debugger and runtime error checking {#debugger-fixes}

One more fix of this area changes what an existing program does:
[Runtime error checking in supervisor mode outside `SetInt`]({{< relref "/docs/compared-with-original/libraries/commands/rundebuglib#supervisor-mode-traps" >}}).

#### `VWait 0` {#vwait-zero}

**What happened.** `VWait 0`, also with a variable that is 0, did not
return at once: it waited for 65536 frames, more than 20 minutes, and
the program looked frozen.

**With the port.** With runtime error checking, a count of 0 is the
error "Can't VWait 0 Frames!". Without it, `VWait 0` waits as long as
in the original.

**What to change.** Use a count of 1 or more, or leave out the
`VWait` when nothing is to be waited for.

Issue: [VWait 0 waits 65536 frames (#1201)](https://github.com/fubar-coder/bb2-c/issues/1201).

{{% fixed-bugs-library part="elsewhere" %}}
