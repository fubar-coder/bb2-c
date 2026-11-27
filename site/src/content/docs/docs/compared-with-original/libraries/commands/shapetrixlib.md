---
title: shapetrixlib
description: The bugs of Blitz Basic 2 in the command library shapetrixlib that
  the BB2 C port fixes.
sidebar:
  order: 52
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/shapetrixlib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Shapes and sprites

These are the commands of shapes, their changes such as `Scale` and
`Rotate`, sprites, stencils and the collisions of sprites. One more fix
of them can stop an existing program:
[`LoadShape` refuses pictures too large to blit](/docs/compared-with-original/libraries/commands/shapeslib/#loadshape-blit-size).

* `Rotate` made a rotated shape one pixel too large and lost its edge
  pixels, so a quarter turn of a 21×9 shape gave 10×21; it rounds to the
  nearest pixel and gives 9×21
  ([#1004](https://github.com/fubar-coder/bb2-c/issues/1004)).
* `XFlip`, `YFlip`, `Rotate` and `Scale` went wrong on shapes with planes
  of 32 KB or more, or with more than 65535 rows in all planes together:
  `YFlip` flipped nothing, and `XFlip` of a 16×8200 shape of 8 planes
  flipped only 64 rows; they work on the whole shape
  ([#274](https://github.com/fubar-coder/bb2-c/issues/274)).
* `XFlip` of a shape whose width is not a multiple of 16 lets the blitter
  shift the rows and then mirrors them with the processor, which started
  before the blitter had finished, so the result depended on the timing;
  the processor waits for the blitter
  ([#271](https://github.com/fubar-coder/bb2-c/issues/271)).
* `Scale` to a width or height of less than one pixel made that one pixel
  from more pixels than the shape has, from the next rows and the memory
  behind the shape, and could give it the wrong colour; it is made from
  the pixels of the shape only
  ([#273](https://github.com/fubar-coder/bb2-c/issues/273)).

::fixed-bugs-library{part="elsewhere"}
