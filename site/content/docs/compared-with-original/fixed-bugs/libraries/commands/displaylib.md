---
title: displaylib
weight: 19
description: The bugs of Blitz Basic 2 in the command library displaylib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `CustomSprites` took its offset 16 times too large {#customsprites-ccoffset}

A copper list of the display library can hold custom copper
instructions, reserved with the `numcustoms` of `InitCopList`.
`CustomColors`, `CustomString`, `CustomSprites` and `CustomChunky` fill
them at a line of the display, `CCOffset` instructions into that space;
an instruction is four bytes. `CustomSprites CopList#,CCOffset,YPos,NumSprites`
sets up the further sprites, the channels 8 to 15 of a list with 8.

**What happened.** The original took the `CCOffset` of `CustomSprites`
as a number of 16 instructions: `CustomSprites 0,2,14,2` put its
instructions 32 instructions into the custom space instead of 2, over
those of other custom commands or behind the space. With runtime error
checking, an offset above the sprite maximum of the compiler options
was the error "Value out of Maximum Range".

**With the port.** `CCOffset` counts instructions, as for the other
custom commands and as the command is documented: the line above starts
the sprite instructions 2 instructions into the custom space. An offset
of 0 is not affected, and the offset is not checked against the sprite
maximum.

**What to change.** A program that gives `CustomSprites` a `CCOffset`
other than 0 now puts the sprite instructions at another place. Give
the offset in instructions, counted like those of `CustomColors` and
`CustomString`. To keep the place the original used, multiply the old
offset by 16.

Issue: [CustomSprites misused CCOffset as a sprite object number (#324)](https://github.com/fubar-coder/bb2-c/issues/324).

## Changes you may notice

### `CustomColors` with a `thru256flag` of 0 acted as 1 {#customcolors-thru256flag}

A copper wait holds only the lower 8 bits of a line, so a custom
command at line 256 of the display or below first adds a wait for the
end of line 255. The last argument of
`CustomColors CopList#,CCOffset,YPos,Palette,startcol,numcols,thru256flag`
tells the display library that the colour change itself runs on past
line 255, as a large AGA palette change can: the custom commands after it
then add no second wait for line 255.

**What happened.** The original did not read the value of the flag: a
`thru256flag` of 0 acted as 1. After

```
InitCopList 0,44,256,3,8,8,20
CustomColors 0,0,20,0,0,1,0
CustomColors 0,4,220,0,0,1
```

the second colour change, at line 264 of the display, had no wait for
line 255, and the copper made it about 256 lines too early.

**With the port.** A `thru256flag` of 0 is the same as leaving the flag
out; any other value sets it. The second colour change above gets its
wait for line 255 and comes at its line.

**What to change.** A program that gives a `thru256flag` of 0 and needs
the flag set has to give 1.

Issue: [CustomColors ignored the value of thru256flag (#951)](https://github.com/fubar-coder/bb2-c/issues/951).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Blitz mode displays

These are the slices and their copper list, the copper lists of the
display library and the fades. Two more fixes of them change what an
existing program shows:
[`CustomSprites` with an offset](#customsprites-ccoffset) and
[`CustomColors` with a `thru256flag` of 0](#customcolors-thru256flag).

* The sprite channels of a copper list that show no sprite, also those of
  `CustomSprites`, had no sprite data address, so the hardware fetched
  their sprites from chip memory address 0 and showed garbage where that
  memory was not 0; they show an empty sprite
  ([#413](https://github.com/fubar-coder/bb2-c/issues/413)).
* `DisplayBitMap CopList#,BitMap#` without a position set the start
  position of `DisplayScroll` to values the program had left over from
  earlier calculations; it starts at 0,0
  ([#411](https://github.com/fubar-coder/bb2-c/issues/411)).
* `CustomChunky` with a split position reset the sprite resolution of
  the copper list, so that `DisplaySprite` then put a high or super high
  resolution sprite with an x handle too far to the left; such sprites
  keep their place
  ([#949](https://github.com/fubar-coder/bb2-c/issues/949)).
* `DisplaySprite` of a sprite on a channel of `CustomSprites` moved
  completely above or below the display wrote the empty sprite over other
  instructions of the copper list, and the sprite stayed visible; the
  sprite is switched off on its own channel
  ([#1051](https://github.com/fubar-coder/bb2-c/issues/1051)).
* `DisplaySprite` of a sprite whose channels run from the last channels
  of the copper list into those of `CustomSprites` wrote over other
  instructions of the list; it is the runtime error "Not Enough Sprite
  Channels", also without runtime error checking
  ([#414](https://github.com/fubar-coder/bb2-c/issues/414);
  [`DisplaySprite` across the two sprite banks]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#displaysprite-two-banks" >}})).

{{% fixed-bugs-library part="elsewhere" %}}
