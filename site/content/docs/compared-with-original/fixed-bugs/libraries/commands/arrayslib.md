---
title: arrayslib
weight: 4
description: The bugs of Blitz Basic 2 in the command library arrayslib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Arrays and sorting

* A `Dim` of an array of 4 GB or more, such as `Dim a.l(32767,32767)`,
  computed a wrong, tiny size and gave an array of a few bytes; it stops
  with the error "Out of Memory!", and with runtime error checking with
  "Array Size out of Range"
  ([#58](https://github.com/fubar-coder/bb2-c/issues/58)).
* A dimension of an array or list has at most 65535 elements, the indexes
  0 to 65534. `Dim a(65535)` and `Dim List a(65535)` gave an array or
  list of no elements, and every access to it lay outside its memory, but
  runtime error checking refused only an index of 65536 and more; it
  refuses 65535 too, with the error "Array Dimension out of Range"
  ([#1037](https://github.com/fubar-coder/bb2-c/issues/1037)).

### Lists

* `Dim List` with size 0 wrote behind its memory block; nothing is
  written there ([#59](https://github.com/fubar-coder/bb2-c/issues/59)).

{{% fixed-bugs-library part="elsewhere" %}}
