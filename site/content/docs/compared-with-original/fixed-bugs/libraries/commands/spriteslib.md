---
title: spriteslib
weight: 55
description: The bugs of Blitz Basic 2 in the command library spriteslib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Shapes and sprites

These are the commands of shapes, their changes such as `Scale` and
`Rotate`, sprites, stencils and the collisions of sprites. One more fix
of them can stop an existing program:
[`LoadShape` refuses pictures too large to blit]({{< relref "/docs/compared-with-original/fixed-bugs/libraries/commands/shapeslib#loadshape-blit-size" >}}).

* `GetaSprite` with runtime error checking had the width limits of 4 and
  16 colour shapes the wrong way round. In `SpriteMode 0` it refused a 4
  colour shape of 65 to 128 pixels, which the 8 sprite channels can show.
  It accepted a 16 colour one of that width, which then failed at every
  `ShowSprite` with "Sprite Channel Unavailable". The port's limits are
  8 sprites (128 pixels in `SpriteMode 0`) for a 4 colour shape and 4
  sprites (64 pixels) for a 16 colour shape; a wider one is the error
  "Sprite too Wide"
  ([#988](https://github.com/fubar-coder/bb2-c/issues/988)).
* `GetaSprite` in `SpriteMode 1` or `2` of a shape whose width is not a
  multiple of the sprite width took bytes of the next row of the shape
  into the right edge of its last sprite; that part of the sprite is
  empty
  ([#282](https://github.com/fubar-coder/bb2-c/issues/282)).
* `SpriteMode` with a mode above 2 took the sprite width from the memory
  behind its table of widths; with runtime error checking it is the error
  "Illegal SpriteMode"
  ([#281](https://github.com/fubar-coder/bb2-c/issues/281)).
* `SaveSprites`, `LoadSprites` and the freeing of a sprite took the size
  of its image from the current `SpriteMode`, so after a change of the
  mode `SaveSprites` wrote too many or too few bytes and `LoadSprites`
  read them so; a sprite keeps the size of the mode it was made in
  ([#280](https://github.com/fubar-coder/bb2-c/issues/280)).
* `ShowSprite` of a sprite cut off at the top of the slice showed the
  wrong lines in `SpriteMode 1` and `2`, and with some values of `Y` read
  the image from anywhere in memory; it skips the lines that are cut off
  ([#283](https://github.com/fubar-coder/bb2-c/issues/283)).

{{% fixed-bugs-library part="elsewhere" %}}
