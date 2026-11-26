---
title: debuglib
weight: 17
description: The bugs of Blitz Basic 2 in the command library debuglib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Debugger and runtime error checking {#debugger-fixes}

One more fix of this area changes what an existing program does:
[Runtime error checking in supervisor mode outside `SetInt`]({{< relref "/docs/compared-with-original/fixed-bugs/libraries/commands/rundebuglib#supervisor-mode-traps" >}}).

* When the debugger evaluated an expression such as a short string, it
  showed the memory behind the string up to 256 characters; it shows only
  the characters of the string
  ([#126](https://github.com/fubar-coder/bb2-c/issues/126)).
* When the current position in a line lay right after a token, for
  example right after `Print`, the debugger marked the end of the line;
  it marks the right column in the source
  ([#127](https://github.com/fubar-coder/bb2-c/issues/127)).

{{% fixed-bugs-library part="elsewhere" %}}
