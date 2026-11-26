---
title: memacclib
weight: 40
description: The bugs of Blitz Basic 2 in the command library memacclib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/memacclib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Strings {#other-strings}

* `Peeks$` with a negative length copied up to 4 GB into the string
  buffer and crashed the program; it returns the empty string, as a
  length of 0 does, and with runtime error checking it is the error
  "Can't Peeks$ a negative number of Characters!"
  ([#214](https://github.com/fubar-coder/bb2-c/issues/214)).

{{% fixed-bugs-library part="elsewhere" %}}
