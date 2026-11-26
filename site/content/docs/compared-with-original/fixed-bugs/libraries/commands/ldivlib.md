---
title: ldivlib
weight: 35
description: The bugs of Blitz Basic 2 in the command library ldivlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Dates and numbers

* A division of longs or quicks by 0 gave the largest positive number for
  every dividend, so -5/0 was 2147483647; it gives the largest number with
  the sign of the dividend, so -5/0 is -2147483647, and it is still no
  error without runtime error checking. With runtime error checking, it
  stops with "GURU! - Division by Zero", as a Word division by 0 does
  ([#200](https://github.com/fubar-coder/bb2-c/issues/200); see
  [A Long or Quick division by 0]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#long-division-by-zero" >}})).

{{% fixed-bugs-library part="elsewhere" %}}
