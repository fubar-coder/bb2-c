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
checking, a shape that one blit cannot copy is an error
([`BlockToShape` with a shape too large for one blit](#blocktoshape-too-large)).

**What to change.** A program that copies shapes wider than 16 pixels
with `BlockToShape` now gets the whole shape. One that relied on the
part the original copied has to copy that part itself.

Issue: [BlockToShape copies one word per line: shapes wider than 16 pixels are copied only in part (#425)](https://github.com/fubar-coder/bb2-c/issues/425).

### `BlockToShape` with a shape too large for one blit {#blocktoshape-too-large}

**What happened.** `BlockToShape` of
[bb_blitlib]({{< relref "/docs/libraries/userlibs/bb_blitlib" >}}) copies
a shape with one blit of as many lines as the shape's height times its
depth. The blitter takes at most 1024 lines and 1024 pixels in one blit.
With a larger shape the size wrapped, and the copy was incomplete without
a message: of a shape 16 by 256 pixels in five planes, 1280 lines, only
256 lines were copied.

**With the port.** With runtime error checking, a shape whose height
times its depth is more than 1024, or one wider than 1024 pixels, is the
error "Shape too large for BlockToShape!". Without runtime error
checking, the copy of such a shape is incomplete, as before.

**What to change.** Keep the shapes you copy with `BlockToShape` within
these limits.

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

#### `CreateShape` with a count of 0 or past the last shape {#createshape-count}

`CreateShape start_shape,count,chipmemblock,like_shape` of
[bb_blitlib]({{< relref "/docs/libraries/userlibs/bb_blitlib" >}}) makes
`count` shapes from `start_shape` on.

**What happened.** The original did not check `count`, also not with
runtime error checking. A count of 0 made 65536 shapes and wrote 2 MB
over the memory behind the shape table, which could crash the machine. A
count that ran past the last shape the Object Maximums of the compiler
options allow wrote the shapes beyond it over the memory behind the
table: with the maximum of 100 shapes, `CreateShape 99,2,m,0` wrote its
second shape there.

**With the port.** With runtime error checking, a count of 0 is the
error "CreateShape count must be at least 1!", and shapes past the last
one the error "CreateShape start+count above Maximum Shapes!". Nothing is
written then. Without runtime error checking, the command works as in the original.

**What to change.** Give a count of at least 1, and raise the maximum of
shapes in the compiler options if the shapes do not fit.

Issues:

* [CreateShape with count 0 creates 65536 shape objects (#428)](https://github.com/fubar-coder/bb2-c/issues/428)
* [CreateShape writes past the shape table when start_shape + count exceeds Maximum Shapes (#1077)](https://github.com/fubar-coder/bb2-c/issues/1077)

#### A shape that was never defined for the Blitz Bombers blits {#bb-blitlib-shape-undefined}

**What happened.** The commands of
[bb_blitlib]({{< relref "/docs/libraries/userlibs/bb_blitlib" >}}) that
blit a shape, `SpecialBlock`, `SpecialBlockBitmap`, `TBlock`,
`PredatorBlit`, `BPredatorBlit`, `BColourBlit` and `SpecialBlockToShape`,
did not check the shape, also not with runtime error checking. A shape
that was never defined has the depth 0, which these commands took as
65536 planes: they blitted over memory all over the system, through
whatever they took for further plane pointers behind the bitmap.

**With the port.** With runtime error checking, such a shape is the
error "Uninitialized Shape", and nothing is blitted. Without runtime
error checking, the commands work as in the original.

**What to change.** Define the shape before you blit it, for example
with `GetaShape` or `LoadShape`.

Issue: [A shape or bitmap of depth 0 makes the plane loops run 65536 times (#429)](https://github.com/fubar-coder/bb2-c/issues/429).

{{% fixed-bugs-library part="elsewhere" %}}
