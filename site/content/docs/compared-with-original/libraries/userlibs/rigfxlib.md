---
title: rigfxlib
weight: 25
description: The bugs of Blitz Basic 2 in the RWE userlib rigfxlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/rigfxlib/]
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `FillPalette` with an end colour of 0 filled nothing {#fillpalette-end-zero}

`FillPalette palette#,r,g,b,start_col,end_col` of
[rigfxlib]({{< relref "/docs/libraries/userlibs/rigfxlib" >}}) sets the
colours `start_col` to `end_col` of a palette object to one colour, and
`AGAFillPalette` does the same with 8 bit values. An `end_col` of 0
means "up to the last colour of the palette", as the library's guide
says.

**What happened.** With an end colour of 0 given as the constant 0 or
as a word variable, the original took colour 0 as the end colour and
filled nothing, or only colour 0 for a `start_col` of 0. Only a long
variable gave the documented result. The reason: the end colour is a
word, but the original tested the whole register it came in for 0.
What the upper half of that register held depended on the code the
compiler had made before the command, and with the constant 0 or a word
variable it was not empty. In this program the first `FillPalette`
sets the colours 2 to 7 of an 8-colour palette to 1, and the second
should set them to 2:

```
DEFTYPE .l
InitPalette 0,8
PaletteInfo 0
FillPalette 0,1,1,1,2,7
FillPalette 0,2,2,2,2,0
NPrint PalRed(1)," ",PalRed(2)," ",PalRed(7)
```

With the original it printed `0 1 1`: the second `FillPalette` did
nothing. With the port it prints `0 2 2`.

**What to change.** Nothing in a program that uses the documented form.
A program that wrote the number of the last colour instead of 0, to get
around the bug, works as before.

Issue: [FillPalette/AGAFillPalette test the whole register of end for 0, but compare only its word (#683)](https://github.com/fubar-coder/bb2-c/issues/683).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rigfxlib {#rigfxlib-fixes}

These fixes concern
[rigfxlib]({{< relref "/docs/libraries/userlibs/rigfxlib" >}}), the
palette and bitmap helpers by Reflective Images. The fix of
[`FillPalette` with an end colour of 0](#fillpalette-end-zero) is
described above.

* `CPUCls` on a bitmap without a first bitplane returns without doing
  anything. The original crashed
  ([#680](https://github.com/fubar-coder/bb2-c/issues/680)).
* `CPUCls` on a bitmap of less than 40 bytes per bitplane, such as one
  of 16 x 16 pixels, clears only its bitplanes. The original cleared
  2.6 MB of memory below each of them
  ([#682](https://github.com/fubar-coder/bb2-c/issues/682)).
* `CPUCls` clears the whole bitplane also when its size in bytes is odd.
  The original left its first byte, 8 pixels at the top left
  ([#681](https://github.com/fubar-coder/bb2-c/issues/681)).
* `CPUCls` reads the depth of the bitmap alone. The original read it
  together with the bitmap's flags, so a bitmap whose flags are set
  counted as one with hundreds of bitplanes. With runtime error
  checking, a bitmap of depth 0 or more than 8 stops with "Illegal
  BitMap Depth"
  ([Bitmap depths for the effects of rifxlib and `CPUCls`]({{< relref "/docs/compared-with-original/libraries/userlibs/rifxlib#rwe-effects-depth" >}}),
  [#679](https://github.com/fubar-coder/bb2-c/issues/679)).
* `CopyColour` with a negative colour number fails and returns 0, as
  for a colour beyond the palette. The original copied from or to
  memory up to 786 KB away from the palette and returned -1
  ([#678](https://github.com/fubar-coder/bb2-c/issues/678)).
* `SaveCMAP` writes the pad byte that the IFF standard puts behind a
  chunk of odd length, so a palette with an odd number of colours gives
  a correct IFF file. The original's file was one byte short
  ([#687](https://github.com/fubar-coder/bb2-c/issues/687)).
* `SaveCMAP` of a palette of 0 colours writes a file with an empty
  colour chunk. The original wrote 192 KB over the memory behind its
  buffer ([#688](https://github.com/fubar-coder/bb2-c/issues/688)).
* A program run a second time from the editor starts with no palette
  chosen with `PaletteInfo`. The original kept the one of the run
  before, and the colour functions and `PalAdjust` read that run's
  freed palette
  ([#1125](https://github.com/fubar-coder/bb2-c/issues/1125)).

#### The checks of the palette commands of rigfxlib {#rigfxlib-palette-checks}

The functions of
[rigfxlib]({{< relref "/docs/libraries/userlibs/rigfxlib" >}}) that read
a colour, `PalRed`, `AGAPalRed` and the others, and `PalAdjust` work on
the palette chosen with `PaletteInfo`. `PalAdjust` writes the adjusted
colours into a target palette, all of them or those from `start_col`
to `end_col`.

**What happened.** The original checked none of the arguments, also not
with runtime error checking. Before the first `PaletteInfo` the
commands read the colours from address 0. A colour number beyond the
palette read, or for `PalAdjust` wrote, the memory behind its colours,
and one from 2731 on the memory before them. `PalAdjust` with an end
colour one below the start colour wrote 256 KB of colours, and without
a range it wrote as many colours as the palette of `PaletteInfo` has,
also into a smaller target palette.

**With the port.** With runtime error checking, these are runtime
errors:

* a colour function or `PalAdjust` before `PaletteInfo`: "No palette
  selected with PaletteInfo";
* a colour number the palette of `PaletteInfo` does not have, also as
  `start_col` or `end_col` of `PalAdjust`: "Colour number outside the
  PaletteInfo palette";
* an `end_col` of `PalAdjust` below its `start_col`: "PalAdjust end
  colour before the start colour";
* a target palette of `PalAdjust` with fewer colours than it writes:
  "PalAdjust colours outside the target palette".

Without runtime error checking, the commands work as in the original, so they
stay as fast as in the original, for example when `PalAdjust` fades a
palette in every frame.

**What to change.** Choose the palette with `PaletteInfo` first, ask
only for the colours it has, and give `PalAdjust` a target palette at
least as large as the colours it writes, as the library's guide asks.

Issues:

* [PalRed/…/AGAPalBlue and PalAdjust read from address 0 before PaletteInfo (#686)](https://github.com/fubar-coder/bb2-c/issues/686)
* [Colour numbers from 2731 on index before the palette (12 × colour used as a signed word) (#677)](https://github.com/fubar-coder/bb2-c/issues/677)
* [PalAdjust with end < start runs up to 65536 × 3 channels (#684)](https://github.com/fubar-coder/bb2-c/issues/684)
* [PalAdjust without start/end adjusts as many colours as PaletteInfo's palette has, not the target (#685)](https://github.com/fubar-coder/bb2-c/issues/685)

{{% fixed-bugs-library part="elsewhere" %}}
