---
title: risortlib
weight: 41
description: The bugs of Blitz Basic 2 in the RWE userlib risortlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/risortlib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in risortlib {#risortlib-fixes}

These fixes concern
[risortlib]({{< relref "/docs/libraries/userlibs/risortlib" >}}), the
sorting of linked lists by a string field of their items by Reflective
Images.

* `StringSort` and `StringSortItem` with `SortPriorityOffset` sort the
  list by priority. After the original had exchanged two items because
  of their priority, it compared the following items with the wrong
  string and the old priority, so the list came out unsorted
  ([#826](https://github.com/fubar-coder/bb2-c/issues/826)).
* `StringSort` and `StringSortItem` with a `sizeof.type` of 0 exchange
  nothing and return 0; the original exchanged 65536 bytes per exchange
  and overwrote the items behind and the memory behind the list
  ([#827](https://github.com/fubar-coder/bb2-c/issues/827)).
* Without `SortPriorityOffset`, the sorts no longer read a priority
  from an odd address, which stopped a plain 68000 with an address
  error ([#825](https://github.com/fubar-coder/bb2-c/issues/825)).
* A program with risortlib no longer crashes at its end on Kickstart
  1.x, which has no utility.library; the original closed the library
  without checking that it had opened it
  ([#824](https://github.com/fubar-coder/bb2-c/issues/824)).

{{% fixed-bugs-library part="elsewhere" %}}
