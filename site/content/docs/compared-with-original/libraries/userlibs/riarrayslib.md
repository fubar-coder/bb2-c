---
title: riarrayslib
weight: 9
description: The bugs of Blitz Basic 2 in the RWE userlib riarrayslib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/riarrayslib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in riarrayslib {#riarrayslib-fixes}

This fix concerns
[riarrayslib]({{< relref "/docs/libraries/userlibs/riarrayslib" >}}),
the command `FillArray` by Reflective Images.

* `FillArray` on an array without elements, whose size is 0, writes
  nothing. The original stored a value first and compared with the end
  of the array only afterwards, so it missed the end and wrote on
  through all memory. The command, which is copied into the program at
  each place it is used, is 4 bytes longer
  ([#882](https://github.com/fubar-coder/bb2-c/issues/882)).

{{% fixed-bugs-library part="elsewhere" %}}
