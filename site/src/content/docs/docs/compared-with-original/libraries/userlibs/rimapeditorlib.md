---
title: rimapeditorlib
description: The bugs of Blitz Basic 2 in the RWE userlib rimapeditorlib that
  the BB2 C port fixes.
sidebar:
  order: 31
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/rimapeditorlib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rimapeditorlib {#rimapeditorlib-fixes}

These fixes concern
[rimapeditorlib](/docs/libraries/userlibs/rimapeditorlib/),
the block maps and helpers of Reflective Images' map editor.

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

#### The maps of rimapeditorlib: stencil, data and positions {#rimapeditorlib-map-checks}

`MapCanDraw(shape#,mapx,mapy)` of
[rimapeditorlib](/docs/libraries/userlibs/rimapeditorlib/)
tells whether a shape may be put into a cell of the current map. It
looks up the value of the cell in the stencil, a byte array set with
`SetStencil`. `MapDataAdr(x,y)` returns the address of a cell, and
`MapDataGrab` fills all cells of the current map from memory.

**What happened.** The original checked none of this, also not with
runtime error checking. Before any `SetStencil`, `MapCanDraw` read the
stencil at address 0. A cell value of 32768 or more counted as negative
and read up to 32 KB in front of the stencil. On a map without data,
such as one whose `CreateMap` could not get memory, `MapCanDraw` read
at address 0. `MapCanDraw` let negative positions pass: it read a word
far behind the map, compared it and kept its address for
`MapCanDrawAddr`. `MapDataAdr(x,y)` returned an address for any
position, for one outside the map an address in a later row or behind
the map.

**With the port.** `MapCanDraw` refuses negative positions and returns
-1 for them. With runtime error checking, these are runtime errors:

* `MapCanDraw` before any `SetStencil`: "MapCanDraw needs SetStencil";
* a cell value of 32768 or more, where `MapCanDraw` looks it up in the
  stencil: "Map value above 32767 for the stencil";
* `MapCanDraw` or `MapDataGrab` on a map without data, after
  `Free MapObject` or a `CreateMap` that got no memory: "MapObject has
  no map data";
* `MapDataAdr(x,y)` with a position outside the map: "Map position
  outside the map".

Without runtime error checking, the commands work as in the original, except
that `MapDataGrab` fills nothing into a map without data.

**What to change.** Call `SetStencil` before `MapCanDraw`, keep the
cell values below 32768, and ask `MapDataAdr` only for positions
inside the map.

Issues:

* [MapCanDraw's stencil lookup is unchecked: without SetStencil it reads low memory; map words ≥ $8000 index before the stencil (#893)](https://github.com/fubar-coder/bb2-c/issues/893)
* [MapCanDraw compares x/y signed: negative coordinates pass and index far behind the map data (#892)](https://github.com/fubar-coder/bb2-c/issues/892)

#### `FillBitplane` and `ShapeTo1616` with sizes that do not fit {#rimapeditorlib-size-checks}

`FillBitplane bitmap#,width,height` of
[rimapeditorlib](/docs/libraries/userlibs/rimapeditorlib/)
fills the top left of the first bitplane of a bitmap with a mesh,
`width` rounded down to a multiple of 16 pixels.
`ShapeTo1616 source,dest` copies the top left 16 by 16 pixels of every
plane of a shape into a shape of 16 by 16 pixels.

**What happened.** The original checked neither, also not with
runtime error checking. `FillBitplane` with a width below 16 wrote
65536 words into each row, and with a height of 0 it filled 65536
rows. `ShapeTo1616` from a shape of height or depth 0 copied 65536
rows or planes. It wrote 16 rows for every plane of the source into
the destination whatever its size, so a destination with fewer planes
or rows was overrun. `MapDataGrab` on a map without data, such as one
whose `CreateMap` got no memory or a width or height of 0, took the
size 0 for 65536 and wrote over all memory from address 0.

**With the port.** With runtime error checking, these are runtime
errors:

* a `FillBitplane` width below 16: "FillBitplane width must be at
  least 16";
* a `FillBitplane` height of 0: "FillBitplane height must be at least
  1";
* a `ShapeTo1616` source shape of height or depth 0: "ShapeTo1616
  source of height or depth 0";
* a `ShapeTo1616` destination that is not 16 by 16 pixels with at least
  the depth of the source: "ShapeTo1616 needs a 16 x 16 destination as
  deep as the source".

Without runtime error checking, `FillBitplane` and `ShapeTo1616` are as
before. `MapDataGrab` fills nothing into a map without data; with
runtime error checking that is an error
([The maps of rimapeditorlib: stencil, data and positions](#rimapeditorlib-map-checks)).

**What to change.** Give `FillBitplane` a width of 16 or more and a
height of 1 or more, and give `ShapeTo1616` a destination shape of 16
by 16 pixels with as many planes as the source.

Issues:

* [MapDataGrab, ShapeTo1616 and FillBitplane loop 65536 times for a size of 0 (DBRA) (#895)](https://github.com/fubar-coder/bb2-c/issues/895)
* [ShapeTo1616 writes 16 words per source plane into the destination shape regardless of its size (#900)](https://github.com/fubar-coder/bb2-c/issues/900)

#### `CreateMap` wider than 32767 cells {#createmap-width}

`CreateMap` of
[rimapeditorlib](/docs/libraries/userlibs/rimapeditorlib/)
makes a map of `width` by `height` cells of one word each.

**What happened.** The original computed the size of a row in 16 bits,
so a map wider than 32767 cells got far too little memory: a width of
40000 got 14464 bytes per row instead of 80000. `MapDataGrab` then
wrote behind the map's data, and `MapCanDraw` read there.

**With the port.** With runtime error checking, a width above 32767 is
the error "Map width above 32767". Without runtime error checking,
`CreateMap` is as before.

**What to change.** Keep maps at 32767 cells wide or less.

Issue: [CreateMap with a width above 32767 gets too little data: MapDataGrab writes behind the map (#1152)](https://github.com/fubar-coder/bb2-c/issues/1152).

::fixed-bugs-library{part="elsewhere"}
