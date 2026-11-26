---
title: banklib
weight: 7
description: The bugs of Blitz Basic 2 in the command library banklib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Files and banks

* `LoadBank` of a drawer could turn the bank into one of size -1 at
  another address; a drawer leaves the bank as it is, like a missing file
  ([#72](https://github.com/fubar-coder/bb2-c/issues/72)).

{{% fixed-bugs-library part="elsewhere" %}}
