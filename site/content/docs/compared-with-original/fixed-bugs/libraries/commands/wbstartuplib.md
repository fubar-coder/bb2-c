---
title: wbstartuplib
weight: 62
description: The bugs of Blitz Basic 2 in the command library wbstartuplib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Program start

* A program started from the CLI with an argument line longer than 255
  characters copied the whole line into a buffer of 256 bytes and
  overwrote the memory behind it; the program gets the first 255
  characters
  ([#308](https://github.com/fubar-coder/bb2-c/issues/308)).
* A program started from the Workbench makes its drawer the current
  directory, and did not make the old current directory current again
  at its end; it does
  ([#307](https://github.com/fubar-coder/bb2-c/issues/307)).
* A program started from the Workbench opened `dos.library` and never
  closed it, so every start raised its use count for good; it is closed
  at the end
  ([#306](https://github.com/fubar-coder/bb2-c/issues/306)).

{{% fixed-bugs-library part="elsewhere" %}}
