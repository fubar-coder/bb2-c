---
title: riencryptlib
weight: 20
description: The bugs of Blitz Basic 2 in the RWE userlib riencryptlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/riencryptlib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in riencryptlib {#riencryptlib-fixes}

These fixes concern
[riencryptlib]({{< relref "/docs/libraries/userlibs/riencryptlib" >}}),
the three-wheel cipher by Reflective Images.

* `Decrypt` with `mode` 1 restores the data for every start position
  and length. It works out where the wheels stopped from the length,
  and the original did so without the carry from one wheel to the next:
  for many start positions and lengths, a wheel came out one step off,
  and the data stayed scrambled
  ([#629](https://github.com/fubar-coder/bb2-c/issues/629)).
* `Encrypt` and `Decrypt` with a length of 0 change nothing; the
  original changed one byte, `Encrypt` the one at the address and
  `Decrypt` the one in front of it
  ([#630](https://github.com/fubar-coder/bb2-c/issues/630)).

{{% fixed-bugs-library part="elsewhere" %}}
