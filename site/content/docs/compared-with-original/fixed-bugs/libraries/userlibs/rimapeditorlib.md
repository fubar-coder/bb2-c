---
title: rimapeditorlib
weight: 31
description: The bugs of Blitz Basic 2 in the RWE userlib rimapeditorlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rimapeditorlib {#rimapeditorlib-fixes}

These fixes concern
[rimapeditorlib]({{< relref "/docs/libraries/userlibs/rimapeditorlib" >}}),
the block maps and helpers of Reflective Images' map editor.

* `MapCanDraw` refuses negative positions and returns -1 for them. The
  original let them pass: it read a word far behind the map, compared
  it, and kept its address for `MapCanDrawAddr`
  ([#892](https://github.com/fubar-coder/bb2-c/issues/892)).
* With runtime error checking, `MapDataAdr(x,y)` with a position outside
  the map stops with "Map position outside the map". The original
  returned an address in a later row or behind the map
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#rimapeditorlib-map-checks" >}}),
  [#892](https://github.com/fubar-coder/bb2-c/issues/892)).
* With runtime error checking, `MapCanDraw` before any `SetStencil`
  stops with "MapCanDraw needs SetStencil", a cell value of 32768 or
  more that it looks up in the stencil with "Map value above 32767 for
  the stencil", and `MapCanDraw` and `MapDataGrab` on a map without
  data with "MapObject has no map data". The original read the stencil
  at address 0 or up to 32 KB in front of it, and the map at address 0
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#rimapeditorlib-map-checks" >}}),
  [#893](https://github.com/fubar-coder/bb2-c/issues/893)).
* `MapDataGrab` fills nothing into a map without data, such as one whose
  `CreateMap` got no memory or a width or height of 0. The original
  took the size 0 for 65536 and wrote over all memory from address 0.
  With runtime error checking, `FillBitplane` with a width below 16 or
  a height of 0 and `ShapeTo1616` from a shape of height or depth 0
  stop with a runtime error; the original wrote 65536 words, rows or
  planes over memory
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#rimapeditorlib-size-checks" >}}),
  [#895](https://github.com/fubar-coder/bb2-c/issues/895)).
* With runtime error checking, `ShapeTo1616` into a destination that is
  not 16 by 16 pixels with at least the depth of the source stops with
  "ShapeTo1616 needs a 16 x 16 destination as deep as the source". The
  original wrote 16 rows for every plane of the source into any
  destination and overran a smaller one
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#rimapeditorlib-size-checks" >}}),
  [#900](https://github.com/fubar-coder/bb2-c/issues/900)).
* With runtime error checking, `CreateMap` with a width above 32767
  stops with "Map width above 32767". The original gave such a map far
  too little memory, and `MapDataGrab` wrote behind it
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#createmap-width" >}}),
  [#1152](https://github.com/fubar-coder/bb2-c/issues/1152)).
* `CreateMap` frees the map the object held wherever its data lie. The
  original did not free a map whose data began at an address that is a
  multiple of 65536, and that memory was lost
  ([#894](https://github.com/fubar-coder/bb2-c/issues/894)).
* `MapDataGrab` with one byte per cell gives each cell its byte plus
  `mapnull`. In the original, a byte plus `mapnull` above 255 added 256
  to all following cells
  ([#898](https://github.com/fubar-coder/bb2-c/issues/898)).
* `MapObjName$` returns a name of 256 characters or more whole. The
  original returned only as many characters as the length modulo 256
  ([#899](https://github.com/fubar-coder/bb2-c/issues/899)).
* `M_InitParse` takes at most 255 characters and cuts a longer text.
  The original copied it whole into its buffer of 256 bytes, so a text
  of 256 characters or more overwrote the library's data behind it, and
  one of 260 or more its code
  ([#897](https://github.com/fubar-coder/bb2-c/issues/897)).
* A program run a second time from the editor starts without a stencil,
  without the results of `ScanShapes`, and with `M_NextArg$` giving an
  empty string before `M_InitParse`. The original kept them from the run
  before, and its stencil had been freed
  ([#1133](https://github.com/fubar-coder/bb2-c/issues/1133)).

{{% fixed-bugs-library part="elsewhere" %}}
