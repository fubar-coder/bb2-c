---
title: spriteslib
description: The bugs of Blitz Basic 2 in the command library spriteslib that
  the BB2 C port fixes.
sidebar:
  order: 55
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/spriteslib/
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

* `GetaSprite` in `SpriteMode 1` or `2` of a shape whose width is not a
  multiple of the sprite width took bytes of the next row of the shape
  into the right edge of its last sprite; that part of the sprite is
  empty
  ([#282](https://github.com/fubar-coder/bb2-c/issues/282)).
* `SaveSprites`, `LoadSprites` and the freeing of a sprite took the size
  of its image from the current `SpriteMode`, so after a change of the
  mode `SaveSprites` wrote too many or too few bytes and `LoadSprites`
  read them so; a sprite keeps the size of the mode it was made in
  ([#280](https://github.com/fubar-coder/bb2-c/issues/280)).
* `ShowSprite` of a sprite cut off at the top of the slice showed the
  wrong lines in `SpriteMode 1` and `2`, and with some values of `Y` read
  the image from anywhere in memory; it skips the lines that are cut off
  ([#283](https://github.com/fubar-coder/bb2-c/issues/283)).

#### `GetaSprite` with a shape too wide for the sprite channels {#getasprite-width}

**What happened.** With runtime error checking, `GetaSprite` refuses a
shape too wide for the 8 sprite channels with "Sprite too Wide". The
limits of the original were the wrong way round: it allowed 4 sprite
widths for a 4 colour shape and 8 for a 16 colour one, which takes two
channels per sprite. So in `SpriteMode 0` a 4 colour shape of 65 to 128
pixels was refused, although it fits, and a 16 colour one of that width
was accepted and failed at every `ShowSprite` with "Sprite Channel
Unavailable".

**With the port.** A 4 colour shape may be 8 sprite widths wide, a 16
colour shape 4: in `SpriteMode 0`, 128 and 64 pixels, in `SpriteMode 1`
256 and 128, in `SpriteMode 2` 512 and 256. A 16 colour shape of 65 to
128 pixels in `SpriteMode 0` is the error "Sprite too Wide"; a 4
colour one of that width is accepted.

**What to change.** Cut a wider 16 colour shape into several sprites, or
use a wider `SpriteMode`.

Issue: ["Sprite too Wide" had the limits of 4 and 16 colour shapes the wrong way round (#988)](https://github.com/fubar-coder/bb2-c/issues/988).

#### `SpriteMode` above 2 {#spritemode-range}

**What happened.** `SpriteMode` takes 0, 1 or 2 for sprites of 16, 32 or
64 pixels. A higher mode took the width from the memory behind the
library's table of widths, and the sprites made after it had a wrong
size.

**With the port.** With runtime error checking, a mode above 2 is the
error "Illegal SpriteMode".

**What to change.** Give a mode from 0 to 2.

Issue: [SpriteMode above 2 read past the width table (#281)](https://github.com/fubar-coder/bb2-c/issues/281).

::fixed-bugs-library{part="elsewhere"}
