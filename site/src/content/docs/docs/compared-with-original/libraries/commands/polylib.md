---
title: polylib
description: The bugs of Blitz Basic 2 in the command library polylib that the
  BB2 C port fixes.
sidebar:
  order: 45
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/polylib/
---

::fixed-bugs-library

## Changes for existing programs

### `RenderToBitmap` with a dither bitmap drew on the wrong bitmap {#rendertobitmap-dither}

`RenderToBitmap BitMap#,DitherBitMap#` makes `RenderPoly` draw on
`BitMap#`, filled with the pattern of `DitherBitMap#`, as `BitMapPoly`
does with a dither bitmap.

**What happened.** The original took the two bitmaps the other way
round: after

```
RenderToBitmap 0,1
RenderPoly 3,&pts(0),2
```

it drew the polygon on bitmap 1, with bitmap 0 as the pattern, and left
bitmap 0 unchanged. A program that wanted to draw on bitmap 0 had to write
`RenderToBitmap 1,0`.

**With the port.** `RenderPoly` draws on the first bitmap with the second
as the pattern, as the command is documented: the lines above draw on
bitmap 0. `RenderToBitmap` with one bitmap is not affected.

**What to change.** A program that swapped the two arguments to get the
polygon on the right bitmap now draws on the dither bitmap. Write the
bitmap to draw on first and the dither bitmap second.

Issue: [RenderToBitmap b,d and RenderPoly drew on the dither bitmap (#421)](https://github.com/fubar-coder/bb2-c/issues/421).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Drawing and polygons

These are the drawing commands on bitmaps and the polygon commands for 3D
graphics. Two more fixes of them change what an existing program draws:
[`RenderToBitmap` with a dither bitmap](#rendertobitmap-dither) and
[`Box` with `BitMapOrigin`](/docs/compared-with-original/libraries/commands/2dlib/#box-origin).

* `ChunkyVlin` with `CLIP` drew a line at an x at or beyond the width of
  the chunky area into other rows or behind the area; such a line is
  clipped away, as `ChunkyPlot` with `CLIP` does
  ([#987](https://github.com/fubar-coder/bb2-c/issues/987)).
* `RenderPoly` before any `RenderToBitmap` or `RenderToChunky` drew the
  polygon into memory it took for a bitmap; it draws nothing
  ([#420](https://github.com/fubar-coder/bb2-c/issues/420)).
* The dither form of `BitMapPoly` and `RenderPoly` took the planes from
  the second one on from behind the dither bitmap when its planes were
  smaller than those of the bitmap drawn on; every plane comes from the
  dither bitmap
  ([#1035](https://github.com/fubar-coder/bb2-c/issues/1035)).

#### `ChunkyVlin` with the ends the wrong way round {#chunkyvlin-ends}

**What happened.** `ChunkyVlin x,y1,y2,colour` without `CLIP` draws from
`y1` down to `y2`. With `y1` greater than `y2` it wrote about 65536 words,
one in each chunky line, far beyond the chunky area.

**With the port.** With runtime error checking, this is the error
"ChunkyVlin y1 MUST NOT be greater than y2". The form with `CLIP` takes
the ends in either order.

**What to change.** Give the upper end first, or use the form with
`CLIP`.

Issue: [ChunkyVlin with y2 below y1 wrote about 65536 words (#417)](https://github.com/fubar-coder/bb2-c/issues/417).

#### `RenderPoly` with a colour above 14 on a chunky area {#renderpoly-colour}

**What happened.** After `RenderToChunky`, the colour of `RenderPoly` is
a number from 0 to 14 for a fixed set of 15 colours. A colour above 14 was
taken from the memory behind that set.

**With the port.** With runtime error checking, this is the error
"Illegal Colour".

**What to change.** Give a colour from 0 to 14.

Issue: [RenderPoly after RenderToChunky read colours above 14 behind its table (#422)](https://github.com/fubar-coder/bb2-c/issues/422).

::fixed-bugs-library{part="elsewhere"}
