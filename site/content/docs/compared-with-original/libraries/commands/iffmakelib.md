---
title: iffmakelib
weight: 30
description: The bugs of Blitz Basic 2 in the command library iffmakelib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/iffmakelib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Bitmaps

* `SaveBitmap` of a bitmap whose flags are set (not one made with
  `BitMap`, which sets none) read the depth together with the flags and
  wrote hundreds of planes per line, from memory behind the bitmap, into
  an invalid file; it writes the planes the bitmap has
  ([#180](https://github.com/fubar-coder/bb2-c/issues/180)).

### IFF files

These are the commands that load and save IFF files, the Amiga's standard
format for pictures (ILBM), colours, sounds and animations. The first two
fixes change the files that `SaveBitmap`, `SaveScreen`, `SaveShape` and
`SavePalette` write, so such a file is not always the same byte for byte
as the one the original writes. Loaded again, it gives the same picture
and the same colours.

* With a palette of an odd number of colours, the colour chunk (CMAP)
  has an odd length and got no pad byte, which IFF requires after such a
  chunk. The picture data behind it then started at an odd place in the
  file, and strict IFF readers, such as programs that use
  `iffparse.library`, could not read the file. The chunk gets its pad
  byte, so these files are one byte longer; the port's load commands
  still read the files without it
  ([#184](https://github.com/fubar-coder/bb2-c/issues/184)).
* The packer of `SaveBitmap`, `SaveScreen` and `SaveShape` looked at the
  byte behind the end of each row, and when that byte equalled the row's
  last two bytes, it packed them differently. Both packings give the same
  picture, but one depended on the next row or on the memory behind the
  bitmap. Every row is packed from its own bytes, so a saved row can
  differ from the original's
  ([#183](https://github.com/fubar-coder/bb2-c/issues/183)).

#### A palette of 0 colours for `SaveBitmap` and `SaveShape` {#save-palette-no-colours}

**What happened.** `InitPalette p,0` makes a palette without colours.
`SaveBitmap` and `SaveShape` with such a palette wrote 65536 colours,
192 KB, into a buffer of 4 KB and over the memory behind it, also with
runtime error checking.

**With the port.** With runtime error checking, a palette without
colours is the error "Uninitialized Palette", as a palette that was never
made. Without runtime error checking, the commands work as in the
original.

**What to change.** Give the palette at least one colour, or save the
picture without a palette.

Issue: [A palette with 0 colours wrote 65536 colours (#182)](https://github.com/fubar-coder/bb2-c/issues/182).

#### `SaveShape` of a shape of depth 0 or more than 14 {#saveshape-depth}

**What happened.** `SaveShape` has room for the addresses of 14 planes.
A shape of more planes, or of depth 0, made it write plane addresses over
the memory of the library, and with depth 0 over 256 KB of it, so the
program crashed or saved a broken file. `LoadShape` takes the depth of a
picture as it is, so a shape loaded from a 24-bit picture has 24 planes.

**With the port.** `SaveShape` of a shape of depth 0 or of more than 14
planes is the runtime error "Illegal Shape Depth", also without runtime
error checking. Shapes of 1 to 14 planes are saved as by the original.

**What to change.** Save such a picture another way, or reduce it to at
most 14 planes before it is loaded as a shape.

Issue: [SaveShape with depth 0 or 15 and more overwrote its buffer (#326)](https://github.com/fubar-coder/bb2-c/issues/326).

{{% fixed-bugs-library part="elsewhere" %}}
