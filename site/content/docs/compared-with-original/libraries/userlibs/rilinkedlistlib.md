---
title: rilinkedlistlib
weight: 29
description: The bugs of Blitz Basic 2 in the RWE userlib rilinkedlistlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/rilinkedlistlib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rilinkedlistlib {#rilinkedlistlib-fixes}

These fixes concern
[rilinkedlistlib]({{< relref "/docs/libraries/userlibs/rilinkedlistlib" >}}),
the five linked lists of copied variables by Steven Matty. Each item of
a list links to the item before and the item after it, and can hold a
copy of a variable.

The links of a list:

* `AddFirstListItem` links the old first item back to the new one. The
  original wrote the new item into the old first item's link to the
  next one, so that item lost the rest of the list, and `NextListItem`
  from the first item went back and forth between the two
  ([#724](https://github.com/fubar-coder/bb2-c/issues/724)).
* `AddListItem` in the middle of a list links the item behind the new
  one back to it; the original left that link on the old current item,
  so `PrevListItem` from there skipped the new item
  ([#727](https://github.com/fubar-coder/bb2-c/issues/727)).
* `DelListItem` of the first item clears the backward link of the new
  first item; the original cleared its forward link instead, so the list
  ended at the new first item
  ([#728](https://github.com/fubar-coder/bb2-c/issues/728)).

New items and results:

* `AddFirstListItem` returns -1 on success, as documented; the original
  returned four times the list number, so 0, the result for a failure,
  on list 0 ([#725](https://github.com/fubar-coder/bb2-c/issues/725)).
* `AddFirstListItem` clears the new item; the original left whatever
  the memory held in it, so the item seemed to hold a copy, and
  `PrevListItem`, `GetListItem` and `DelListItem` used meaningless
  addresses ([#726](https://github.com/fubar-coder/bb2-c/issues/726)).
* On a list that was never started with `InitList`, the commands do
  nothing and return 0, as documented. Only `DelListItem` checked this
  in the original: the others read and wrote memory from address 0 on,
  and `FirstListItem` and `LastListItem` returned -1
  ([#733](https://github.com/fubar-coder/bb2-c/issues/733)).
* With runtime error checking, a list number outside 0 to 4 stops with
  the error "List number must be 0 to 4"; the original wrote into the
  other lists and the memory behind them
  ([A list number outside 0 to 4]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#linked-list-number-range" >}}),
  [#732](https://github.com/fubar-coder/bb2-c/issues/732)).

The copies of the items:

* `DelListItem` frees the item with its copy. The original read the
  copy's address from the wrong place and freed a wrong address: the
  program stopped with "Unable to Free Memory", or crashed without
  runtime error checking
  ([#729](https://github.com/fubar-coder/bb2-c/issues/729)).
* `SetListItem` frees the copy the item held before; the original lost
  it until the program ended
  ([#734](https://github.com/fubar-coder/bb2-c/issues/734)).
* `InitList` on a list that is already there frees it first, with the
  copies of its items; the original lost the old list until the program
  ended ([#731](https://github.com/fubar-coder/bb2-c/issues/731)).
* `GetListItem` copies no more bytes than the item's copy has, and the
  rest of a larger variable stays as it is; the original filled it with
  the bytes behind the copy
  ([#730](https://github.com/fubar-coder/bb2-c/issues/730)).
* The size an item keeps of its copy is right after `AddListItem`,
  `AddLastListItem` and `SetListItem`; the original stored the size of
  the last variable any command had looked at, or kept the old size
  ([#735](https://github.com/fubar-coder/bb2-c/issues/735)).

{{% fixed-bugs-library part="elsewhere" %}}
