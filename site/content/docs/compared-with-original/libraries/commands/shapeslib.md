---
title: shapeslib
weight: 51
description: The bugs of Blitz Basic 2 in the command library shapeslib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/shapeslib/]
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `LoadShape` refuses pictures too large to blit {#loadshape-blit-size}

The blitter draws a shape in one go, with its size given as one word: 10
bits for the height and 6 for the width in 16 pixel words. A shape
keeps that word, so it can be at most 1023 rows high and 62 words, 992
pixels, wide.

**What happened.** `LoadShape` loaded a taller or wider picture all the
same. The size word wrapped, and every blit of the shape drew a wrong
part of it: a picture 1100 rows high was blitted 76 rows high.

**With the port.** `LoadShape` of a picture taller than 1023 rows or
wider than 992 pixels is the runtime error "Can't Read/Write Shapes
File", also without runtime error checking, and the shape stays empty.
This can stop a program that ran with the original: one that loaded
such a picture as a shape only to read its size with `ShapeWidth` and
`ShapeHeight`, and never blitted it.

**What to change.** Load such a picture as a bitmap, with `LoadBitMap`,
or cut it into shapes of at most 1023 rows and 992 pixels
([`LoadShape` of a picture too large to blit]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#loadshape-blit-size" >}})).

**A plane just under 64 KB.** A shape also keeps the size of one plane
with an extra word per row in 16 bits: `BBlit` saves that many bytes of
the background per plane. The original kept only the lower 16 bits of it
for a picture whose plane alone is just under 64 KB, so `BBlit` saved and
restored the wrong amount. `LoadShape` refuses such a picture with the
same runtime error, as `InitShape` and `GetaShape` refuse a shape of its
size with runtime error checking.

The rule for a shape is that the size of one plane with its extra word
per row, (⌈width / 16⌉ × 2 + 2) × height bytes, must stay below 65536
(64 KB). A
picture of 992 by 520 pixels loads; one of 992 by 528 is refused.

Only pictures 512 to 992 pixels wide and 521 to 1023 rows high can fall
into this band. A program that loaded one and only blitted it with `Blit`
stops here too
([`LoadShape` of a picture with very large planes]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#loadshape-large-plane" >}})).

Issues:

* [The blit size word of a shape taller than 1023 rows overflows (#1046)](https://github.com/fubar-coder/bb2-c/issues/1046)
* [LoadShape and the shape creation checks use different 64 KB plane limits (#1055)](https://github.com/fubar-coder/bb2-c/issues/1055)

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Shapes and sprites

These are the commands of shapes, their changes such as `Scale` and
`Rotate`, sprites, stencils and the collisions of sprites. One more fix
of them can stop an existing program:
[`LoadShape` refuses pictures too large to blit](#loadshape-blit-size).

* `LoadShapes`, `DecodeShapes` and `LoadSprites` with a raised object
  maximum (1025 shapes or 2049 sprites and more) loaded only the first
  object; they load the whole range
  ([#999](https://github.com/fubar-coder/bb2-c/issues/999)).
* `LoadShape` of a picture with planes of 32 KB or more wrote the planes
  from the second one on in front of the shape's memory; they are in
  place ([#263](https://github.com/fubar-coder/bb2-c/issues/263)).
* `SaveShapes`, `LoadShapes`, `DecodeShapes` and `CopyShape` handled only
  part of a shape of 64 KB or more, so a file the original saved from
  such a shape holds only part of it; the whole shape is saved, loaded
  and copied ([#263](https://github.com/fubar-coder/bb2-c/issues/263)).
* A single plane of 64 KB or more does not fit the shapes format, which
  keeps the size of a plane in 16 bits. `LoadShape` of such a picture
  wrote it over the memory behind a far too small shape; such a picture
  is the runtime error "Can't Read/Write Shapes File", also without
  runtime error checking
  ([#263](https://github.com/fubar-coder/bb2-c/issues/263);
  [LoadShape of a picture with very large planes]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#loadshape-large-plane" >}})).
* `LoadShape` of an IFF ANIM put the frames that come after the last
  shape object into the memory behind the table of shapes; it stops at the
  last shape object
  ([#265](https://github.com/fubar-coder/bb2-c/issues/265)).
* `LoadShape` of an IFF ANIM took the XOR mode of the first frame for
  every frame, so an animation that mixes XOR frames and others came out
  wrong; each frame has its own mode
  ([#266](https://github.com/fubar-coder/bb2-c/issues/266)).
* `DecodeShapes` uses shape data that lies in chip memory where it is,
  and marks the shape so that this memory, which the library did not
  allocate, is not freed. The mark stayed when the shape was freed or
  replaced, so the next shape in that object was never freed before the
  program ended; the mark goes with the shape
  ([#269](https://github.com/fubar-coder/bb2-c/issues/269)).
* `CopyShape` of such a shape copied the mark, so the memory of the copy
  was never freed before the program ended; the copy has no mark
  ([#268](https://github.com/fubar-coder/bb2-c/issues/268)).
* A file that `SaveShapes` wrote from such a shape held the mark, and
  `LoadShapes` of the file, or `DecodeShapes` of it outside chip memory,
  took it over, so the loaded shapes were never freed before the program
  ended; the mark is not taken from the file
  ([#267](https://github.com/fubar-coder/bb2-c/issues/267)).
* `LoadShape` of a picture whose header gives 0 bitplanes made a shape
  without memory and wrote the picture data behind it; such a picture is
  the runtime error "Illegal Shape Depth", also without runtime error
  checking
  ([#1052](https://github.com/fubar-coder/bb2-c/issues/1052);
  [`LoadShape` of a picture of 0 bitplanes]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#loadshape-no-planes" >}})).
* `InitShape`, `GetaShape`, `Rotate` and `Scale` made shapes too large
  for the size fields of a shape without a check, so the shape's memory
  was overwritten or blitted wrongly. With runtime error checking, a
  shape with a plane of 64 KB or more, taller than 1023 rows or wider
  than 992 pixels is the error "Shape too Big", and `InitShape` with a
  width or height of 0 the error "Illegal Shape Size"
  ([#1047](https://github.com/fubar-coder/bb2-c/issues/1047),
  [#1046](https://github.com/fubar-coder/bb2-c/issues/1046);
  [Shapes too large for their size fields]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#shape-too-big" >}})).
* `LoadShape` of a picture whose header gives a width or a height of 0
  made a shape without memory and ran its loops 65536 times, writing the
  picture data behind it; such a picture is the runtime error "Illegal
  Shape Size", also without runtime error checking
  ([#1054](https://github.com/fubar-coder/bb2-c/issues/1054);
  [`LoadShape` of a picture of width or height 0]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#loadshape-size-zero" >}})).

### IFF files

These are the commands that load and save IFF files, the Amiga's standard
format for pictures (ILBM), colours, sounds and animations.

* `LoadShape` of an animation that stopped after its first frame with
  "ANIM format not supported" left the file open and locked until the
  next reboot, also when the program went on with `SetErr`; the file is
  closed before the error, and the shape keeps the first frame
  ([#1072](https://github.com/fubar-coder/bb2-c/issues/1072)).

{{% fixed-bugs-library part="elsewhere" %}}
