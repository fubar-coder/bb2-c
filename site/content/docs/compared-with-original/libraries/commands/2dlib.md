---
title: 2dlib
weight: 1
description: The bugs of Blitz Basic 2 in the command library 2dlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/2dlib/]
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `Box` added the bitmap origin twice {#box-origin}

`BitMapOrigin` moves the point 0,0 of a bitmap for the drawing commands.
`Line`, `Boxf` and `Plot` add the origin to their coordinates once;
`Box` added it twice, so after `BitMapOrigin 0,50,50` the line
`Box 0,0,10,10,1` drew its corner at 100,100.

**With the port.** `Box` adds the origin once, like the others: the corner
is at 50,50. Without `BitMapOrigin`, or with the origin 0,0, nothing
changes.

**What to change.** A program that gave `Box` coordinates reduced by the
origin to make up for the doubling now draws the box displaced. Give
`Box` the same coordinates as `Line` and `Boxf`.

Issue: [Box added the bitmap origin twice (#391)](https://github.com/fubar-coder/bb2-c/issues/391).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Drawing and polygons

These are the drawing commands on bitmaps and the polygon commands for 3D
graphics. Two more fixes of them change what an existing program draws:
[`RenderToBitmap` with a dither bitmap]({{< relref "/docs/compared-with-original/libraries/commands/polylib#rendertobitmap-dither" >}}) and
[`Box` with `BitMapOrigin`](#box-origin).

* `Circle` and `Circlef` wrote the points of a circle that reaches above
  the top edge of the bitmap far behind its planes or into the memory
  behind the fill buffer; such points are clipped, like those below the
  bottom edge
  ([#392](https://github.com/fubar-coder/bb2-c/issues/392)).
* `AllocFill` as a statement took the size of the fill buffer from two
  words of the string buffer instead of the used bitmap, so it made a
  buffer of the wrong size or none; it makes the buffer for the used bitmap
  ([#390](https://github.com/fubar-coder/bb2-c/issues/390)).
* `Polyf` drew polygons whose corners lie far outside the bitmap (from
  about 8000 pixels away) wrongly or not at all; they are drawn
  ([#1000](https://github.com/fubar-coder/bb2-c/issues/1000)).
* `Polyf`, `BitMapPoly`, `ChunkyPoly` and `RenderPoly` stopped with a
  "Division by Zero" guru for some degenerate polygons with a side that
  starts outside the bitmap on a vertical edge; such a polygon is not drawn
  ([#1033](https://github.com/fubar-coder/bb2-c/issues/1033)).
* `FloodFill` with a border colour filled wrongly on a bitmap wider than
  1024 pixels or taller than 1024 lines; it fills these bitmaps as it
  fills smaller ones
  ([#986](https://github.com/fubar-coder/bb2-c/issues/986)).

#### `Poly` with fewer than 2 points {#poly-points}

**What happened.** `Poly` draws a line from each point to the next. With
1 point or none it counted down past 0 and drew 65535 or 65536 lines from the
words in memory behind the coordinates.

**With the port.** With runtime error checking, this is the error "Poly
needs at least 2 Points".

**What to change.** Give `Poly` at least 2 points; draw a single point
with `Plot`.

Issue: [Poly with 1 point drew 65535 lines (#396)](https://github.com/fubar-coder/bb2-c/issues/396).

#### A polygon of more than 16 points {#too-many-points}

**What happened.** `Polyf`, `BitMapPoly`, `ChunkyPoly` and `RenderPoly`
take at most 16 points. More points overwrote the code of the library,
and the program crashed, also with runtime error checking. A polygon of
up to 16 points was drawn wrongly when one of its sides had 11 to 15
edges.

**With the port.** With runtime error checking, more than 16 points are
the error "Too Many Points"; for the polygon commands that take a
negative count for the points in the other order, so is a count below
-16. A polygon of up to 16 points is drawn right, whatever the number of
edges of its sides.

**What to change.** Split the polygon into polygons of at most 16 points.

Issue: [Polyf, BitMapPoly and ChunkyPoly overflowed with more than 16 points (#395)](https://github.com/fubar-coder/bb2-c/issues/395).

{{% fixed-bugs-library part="elsewhere" %}}
