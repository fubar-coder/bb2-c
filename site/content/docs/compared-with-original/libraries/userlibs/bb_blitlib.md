---
title: bb_blitlib
weight: 1
description: The bugs of Blitz Basic 2 in the RWE userlib bb_blitlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/bb_blitlib/]
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `BlockToShape` copied only part of a wide shape {#blocktoshape-width}

[bb_blitlib]({{< relref "/docs/libraries/userlibs/bb_blitlib" >}}) holds
the blitter routines of the game Blitz Bombers. Its `BlockToShape`
copies the data of one shape into another shape of the same size.

**What happened.** The command copied one word, 16 pixels, of every line
of the shape, whatever the shape's width. The planes of a shape lie one
after another, so of a wider shape only the first part of its data
reached the other shape: of a shape 32 pixels wide the first half, its
first planes. This program copies a box of colour 15, 32 by 16 pixels in
four planes, and draws the copy:

```
BitMap 0,320,64,4
Boxf 0,0,31,15,15
GetaShape 0,0,0,32,16
GetaShape 1,0,20,32,16
OwnBlitter_
BlockToShape 0,1
WaitBlit_
DisownBlitter_
Cls
Block 1,0,0
WaitBlit_
NPrint Point(8,8)," ",Point(24,8)
End
```

**With the original**

```
3 3
```

**With the port**

```
15 15
```

The original copied two of the four planes, so the copy came out in
colour 3. The port copies the whole shape, whatever its width; shapes
16 pixels wide are copied as before. With runtime error
checking, a shape that one blit cannot copy, one whose height times its
depth is more than 1024 or one wider than 1024 pixels, is the error
"Shape too large for BlockToShape!"
([`BlockToShape` with a shape too large for one blit]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#blocktoshape-too-large" >}})).

**What to change.** A program that copies shapes wider than 16 pixels
with `BlockToShape` now gets the whole shape. One that relied on the
part the original copied has to copy that part itself.

Issue: [BlockToShape copies one word per line: shapes wider than 16 pixels are copied only in part (#425)](https://github.com/fubar-coder/bb2-c/issues/425).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in bb_blitlib {#bb-blitlib-fixes}

These fixes concern
[bb_blitlib]({{< relref "/docs/libraries/userlibs/bb_blitlib" >}}), the
blitter routines of the game Blitz Bombers; the fix of `BlockToShape` is
[described above](#blocktoshape-width).

* `BTBlockImage` takes image 147 and the ones after it from their place
  in the block of images; the original took images 147 to 292 from the
  memory 64 KB before `image_addr`, and the ones from 293 on from inside
  the first images
  ([#427](https://github.com/fubar-coder/bb2-c/issues/427)).
* `BPredatorBlit` finds the line below with the line width of the
  bitmap it draws on; the original used the width set with
  `SetupMapTwo`, so on a bitmap of another width the effect showed
  another part of the background, on a bitmap 640 pixels wide the same
  line 320 pixels further right
  ([#426](https://github.com/fubar-coder/bb2-c/issues/426)).
* With runtime error checking, `CreateShape` with a count of 0 is the
  error "CreateShape count must be at least 1!"; the original made 65536
  shapes and wrote 2 MB over the memory behind the shape table
  ([`CreateShape` with a count of 0 or past the last shape]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#createshape-count" >}}),
  [#428](https://github.com/fubar-coder/bb2-c/issues/428)).
* With runtime error checking, `CreateShape` whose shapes would run past
  the last shape that the Object Maximums of the compiler options allow
  is the error "CreateShape start+count above Maximum Shapes!"; the
  original wrote those shapes over the memory behind the shape table
  ([`CreateShape` with a count of 0 or past the last shape]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#createshape-count" >}}),
  [#1077](https://github.com/fubar-coder/bb2-c/issues/1077)).
* With runtime error checking, the commands that blit a shape,
  `SpecialBlock`, `SpecialBlockBitmap`, `TBlock`, `PredatorBlit`,
  `BPredatorBlit`, `BColourBlit` and `SpecialBlockToShape`, refuse a
  shape that was never defined with "Uninitialized Shape"; with the
  original such a shape, of depth 0, made them blit 65536 planes over
  memory all over the system
  ([A shape that was never defined for the Blitz Bombers blits]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#bb-blitlib-shape-undefined" >}}),
  [#429](https://github.com/fubar-coder/bb2-c/issues/429)).

{{% fixed-bugs-library part="elsewhere" %}}
