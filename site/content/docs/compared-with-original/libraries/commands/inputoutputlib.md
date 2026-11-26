---
title: inputoutputlib
weight: 32
description: The bugs of Blitz Basic 2 in the command library inputoutputlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/inputoutputlib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Input

* The runtime error of `Inkey$` from the default input read "Can't Input
  Character from Defualt Input"; it reads "Default"
  ([#193](https://github.com/fubar-coder/bb2-c/issues/193)).
* With runtime error checking, the first `PopInput` or `PopOutput` with
  no switch of the channel left to undo, such as one at the start of the
  program, passed the check and left the program without a channel; the
  error came only at the pop after it. It is reported at the first such
  pop
  ([#1032](https://github.com/fubar-coder/bb2-c/issues/1032)).

{{% fixed-bugs-library part="elsewhere" %}}
