---
title: mouselib
weight: 43
description: The bugs of Blitz Basic 2 in the command library mouselib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Input

* `Mouse On` took the start values of the two mouse counters from each
  other's place, so the mouse jumped once, by the difference of the
  counters, at the first reading; it starts where it is
  ([#223](https://github.com/fubar-coder/bb2-c/issues/223)).

{{% fixed-bugs-library part="elsewhere" %}}
