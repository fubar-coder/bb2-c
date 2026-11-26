---
title: sortlib
weight: 53
description: The bugs of Blitz Basic 2 in the command library sortlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Arrays and sorting

* `Sort` on a string array read the length of an element that was never
  given a value from address -4 and its characters from address 0, so
  such elements sorted to places that depended on that memory; they
  count as empty strings and come first, or last after `SortDown`
  ([#276](https://github.com/fubar-coder/bb2-c/issues/276)).
* `Sort` on a word array took the word after the middle element as the
  value it compares with, so for an array of one element it read the word
  behind the array and searched the memory behind and possibly before it;
  it takes the middle element. The sorted array is the same
  ([#275](https://github.com/fubar-coder/bb2-c/issues/275)).

{{% fixed-bugs-library part="elsewhere" %}}
