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
part of it: a picture 16 by 1100 pixels was blitted 76 rows high.

**With the port.** `LoadShape` of a picture taller than 1023 rows or
wider than 992 pixels is the runtime error "Can't Read/Write Shapes
File", also without runtime error checking, and the shape stays empty.
This can stop a program that ran with the original: one that loaded
such a picture as a shape only to read its size with `ShapeWidth` and
`ShapeHeight`, and never blitted it.

**What to change.** Load such a picture as a bitmap, with `LoadBitMap`,
or cut it into shapes of at most 1023 rows and 992 pixels.

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
([`LoadShape` of a picture with very large planes](#loadshape-large-plane)).

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

#### `LoadShape` of a picture with very large planes {#loadshape-large-plane}

**What happened.** A shape keeps two sizes in 16 bits each, in memory
and in the files of `SaveShapes`: the size of one plane, and the size of
one plane with an extra word per row. `BBlit` saves that second amount
of the background. `LoadShape` of an IFF picture with a plane of 64 KB or more,
such as 1024 by 520 pixels, took only the lower 16 bits of that size,
made the shape far too small and wrote the whole picture into it, over
the memory behind it. A picture whose plane is just under 64 KB, such as
992 by 528 pixels, loaded, but `BBlit` then saved and restored the wrong
amount of the background.

**With the port.** Such a picture is the runtime error "Can't Read/Write
Shapes File", also without runtime error checking, as for a file that
cannot be read. The shape stays empty. The limit is that of `InitShape`
and `GetaShape` ([Shapes too large for their size fields](#shape-too-big)):
the size of one plane with its extra word per row,
(⌈width / 16⌉ × 2 + 2) × height bytes, must stay below 65536 (64 KB).
A picture of 992 by 520 pixels loads.

**What to change.** Load the picture as a bitmap, or cut it into shapes
within that limit.

Issues:

* [Large shapes had their line and plane steps and sizes only as a word (#263)](https://github.com/fubar-coder/bb2-c/issues/263)
* [LoadShape and the shape creation checks use different 64 KB plane limits (#1055)](https://github.com/fubar-coder/bb2-c/issues/1055)

#### `LoadShape` of a picture of 0 bitplanes {#loadshape-no-planes}

**What happened.** `LoadShape` took the number of bitplanes from the
header of the picture without a check. A header that gives 0 bitplanes,
in a damaged or unusual file, made a shape without memory, and the
picture data was written behind it until the file ended.

**With the port.** Such a picture is the runtime error "Illegal Shape
Depth", also without runtime error checking, and the shape stays empty.

**What to change.** Nothing for a correct picture; a picture of 0
bitplanes holds no image.

Issue: [LoadShape of an ILBM with 0 planes writes the file behind an empty block (#1052)](https://github.com/fubar-coder/bb2-c/issues/1052).

#### Shapes too large for their size fields {#shape-too-big}

**What happened.** A shape keeps the size of a plane, with an extra word
per row, in 16 bits, and the size the blitter draws it with in one word.
`InitShape`, `GetaShape`, `Rotate` and `Scale` made shapes without
checking either: a shape with a plane of 64 KB or more got wrong sizes,
and its memory was overwritten; a shape taller than 1023 rows or wider
than 992 pixels was blitted wrongly. `InitShape` also made a shape of
width or height 0, whose blits ran their loops 65536 times.

**With the port.** With runtime error checking, `InitShape`,
`GetaShape`, `Rotate` and `Scale` stop with "Shape too Big" when the new
shape breaks one of these limits:

* the size of one plane with its extra word per row,
  (⌈width / 16⌉ × 2 + 2) × height bytes, must stay below 65536 (64 KB);
* the height must be at most 1023 rows;
* the width must be at most 992 pixels.

`InitShape` with a width or height of
0 is the error "Illegal Shape Size". Without runtime error checking, the
commands work as in the original.

**What to change.** Keep shapes within these sizes; draw a larger image
on a bitmap.

Issues:

* [InitShape, GetaShape, Rotate and Scale do not check for planes of 64 KB or more (#1047)](https://github.com/fubar-coder/bb2-c/issues/1047)
* [The blit size word of a shape taller than 1023 rows overflows (#1046)](https://github.com/fubar-coder/bb2-c/issues/1046)

#### `LoadShape` of a picture of width or height 0 {#loadshape-size-zero}

**What happened.** `LoadShape` took the width and the height from the
header of the picture without a check. A header that gives a width or a
height of 0 made a shape without memory, and the loops over the bytes or
the rows of the picture ran 65536 times, writing the picture data behind
the shape until the file ended.

**With the port.** Such a picture is the runtime error "Illegal Shape
Size", also without runtime error checking, and the shape stays empty.

**What to change.** Nothing for a correct picture; a picture of width or
height 0 holds no image.

Issue: [LoadShape of an ILBM with width 0 or height 0 runs its loops 65536 times (#1054)](https://github.com/fubar-coder/bb2-c/issues/1054).

### IFF files

These are the commands that load and save IFF files, the Amiga's standard
format for pictures (ILBM), colours, sounds and animations.

* `LoadShape` of an animation that stopped after its first frame with
  "ANIM format not supported" left the file open and locked until the
  next reboot, also when the program went on with `SetErr`; the file is
  closed before the error, and the shape keeps the first frame
  ([#1072](https://github.com/fubar-coder/bb2-c/issues/1072)).

{{% fixed-bugs-library part="elsewhere" %}}
