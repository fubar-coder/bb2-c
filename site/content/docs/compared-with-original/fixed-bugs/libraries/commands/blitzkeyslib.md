---
title: blitzkeyslib
weight: 11
description: The bugs of Blitz Basic 2 in the command library blitzkeyslib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Input

* `BlitzKeys Off` left the input with the keyboard reading of Blitz
  mode, and after a second `BlitzKeys On` reading a line, such as with
  `Edit$`, hung for ever. It gives the keyboard input back to the input
  used before `BlitzKeys On`, unless the program chose another input in
  between, such as with `FileInput`, which stays
  ([#408](https://github.com/fubar-coder/bb2-c/issues/408)).

{{% fixed-bugs-library part="elsewhere" %}}
