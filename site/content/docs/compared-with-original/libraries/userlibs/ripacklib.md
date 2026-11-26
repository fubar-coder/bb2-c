---
title: ripacklib
weight: 35
description: The bugs of Blitz Basic 2 in the RWE userlib ripacklib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/ripacklib/]
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `ILBMPalette` with an offset replaced a palette that was too small {#ilbmpalette-offset}

[ripacklib]({{< relref "/docs/libraries/userlibs/ripacklib" >}})
unpacks IFF ILBM pictures that a program holds in memory.
`ILBMPalette source,palette#,offset` takes the colours of the picture's
CMAP chunk into palette object `palette#` from colour number `offset`
on, so that a program can put the colours of several pictures into one
palette.

**What happened.** When the palette had fewer colours than `offset`
plus the colours of the CMAP, the original threw the palette's colours
away, made a new palette of only the CMAP's colours and put them from
colour 0 on; it still returned -1. A palette of 8 colours and a CMAP of
4 colours loaded at offset 6 gave a palette of 4 colours, the CMAP's,
at 0 to 3.

**With the port.** The palette grows to `offset` plus the colours of
the CMAP and keeps its own colours; colours between its old end and
`offset` are black. In the example the palette has 10 colours: 0 to 5
as they were, 6 to 9 from the CMAP. Without `offset`, the palette is
replaced by the CMAP's colours, as before.

**What to change.** A program that gave an offset to a palette that was
too small and used the colours from 0 on leaves the offset out.

Issue: [ILBMPalette with an offset into a palette that is too small replaces the palette and ignores the offset (#769)](https://github.com/fubar-coder/bb2-c/issues/769).

## Changes you may notice

### `LoadIFF` needed `BankLimit` and reported pictures it had not unpacked {#loadiff-bank-result}

`LoadIFF filename$,bitmap[,palette]` of
[ripacklib]({{< relref "/docs/libraries/userlibs/ripacklib" >}}) loads
an IFF ILBM picture file into a free memory bank of
[riamosfunclib]({{< relref "/docs/libraries/userlibs/riamosfunclib" >}}),
unpacks it into the bitmap object, and into the palette object if one is
given, and frees the bank again.

**What happened.** The original asked for a free bank from bank 51 on.
riamosfunclib has 50 banks, 0 to 49, unless the program raises the
number with `BankLimit`, so without that `LoadIFF` found no bank, loaded
nothing and returned 0. Once it had loaded the file, it returned -1
without looking at the unpacking: a file that is no ILBM picture, or a
picture too wide or too deep for the bitmap, left the bitmap as it was
and still counted as loaded. With a picture file of one plane:

```
BitMap 0,320,16,1
r=LoadIFF("picture.iff",0)
NPrint r," ",Point(0,0)
```

With the original this printed `0 0`: nothing loaded. With the port it
prints `-1 1`, and the picture is in the bitmap.

**With the port.** `LoadIFF` takes the first free bank from bank 0 on,
so it works with the 50 banks riamosfunclib starts with; the banks the
program uses are not touched. It returns -1 when the picture was
unpacked into the bitmap and, with a palette, its colours were taken,
and 0 otherwise.

**What to change.** `BankLimit` is no longer needed for `LoadIFF`, but
does no harm. A picture without colours (no CMAP chunk) loaded with a
palette is unpacked into the bitmap, but `LoadIFF` returns 0; load such
a picture without the palette.

Issues:

* [LoadIFF needs a bank from 51 on, but RIAmosFuncLib has only 50 banks by default (#770)](https://github.com/fubar-coder/bb2-c/issues/770)
* [LoadIFF returns -1 even if UnpackIFF failed (#771)](https://github.com/fubar-coder/bb2-c/issues/771)

### `UnpackIFF` read pictures whose width is no multiple of 16 out of step {#unpackiff-row-width}

An ILBM picture stores every row of a plane padded to whole words of 16
pixels: a picture 40 pixels wide has 6 bytes per row, not 5.
`UnpackIFF` of
[ripacklib]({{< relref "/docs/libraries/userlibs/ripacklib" >}}), and
`ILBMGrab` and `LoadIFF`, which use it, unpack such a picture into a
bitmap object.

**What happened.** The original took the width divided by 8 as the
bytes of a row and so read the padding of each row as the start of the
next one: from the second row on the picture came out shifted and
garbled, and for a width that is no multiple of 8 the last pixels of
every row were missing. Pictures 16, 32, 320 or 640 pixels wide, a
multiple of 16, were not affected.

**With the port.** The rows are read as the file stores them, and such
a picture comes out as it was saved. The bitmap must be as wide as the
padded row; a bitmap made with `BitMap` is, when it is at least as wide
as the picture.

**What to change.** Nothing, unless a program made up for the garbled
rows, for example by saving brushes only in widths that are a multiple
of 16; such brushes load as before.

Issue: [UnpackIFF takes the row width as width/8 instead of the word-aligned ILBM row (#775)](https://github.com/fubar-coder/bb2-c/issues/775).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ripacklib {#ripacklib-fixes}

These fixes concern
[ripacklib]({{< relref "/docs/libraries/userlibs/ripacklib" >}}), the
library by Reflective Images that unpacks IFF ILBM pictures in memory
into bitmaps and palettes, packs and unpacks ByteRun1 data and unpacks
Pack-Ice files. The fixes of
[`ILBMPalette` with an offset](#ilbmpalette-offset), of
[`LoadIFF`'s bank and result](#loadiff-bank-result) and of
[pictures whose width is no multiple of 16](#unpackiff-row-width) are
described above.

* `UnpackIFF` and `ILBMPalette` find the chunks of a picture by walking
  the IFF file from chunk to chunk, also behind large chunks such as a
  long annotation, and no longer take the name of a chunk inside other
  data for the chunk itself. The original searched the raw data, only
  within about 5 KB, and missed chunks further on
  ([#766](https://github.com/fubar-coder/bb2-c/issues/766)).
* `UnpackIFF` reads the compression byte of the picture: an uncompressed
  picture is copied as it is. The original unpacked every picture as
  ByteRun1, so an uncompressed one came out as garbage
  ([#773](https://github.com/fubar-coder/bb2-c/issues/773)).
* `UnpackIFF` unpacks pictures wider than 1024 pixels. The original
  counted the bytes of a row in a byte: such pictures were not unpacked,
  or read out of step, though the command reported success
  ([#774](https://github.com/fubar-coder/bb2-c/issues/774)).
* `UnpackIFF` cuts a ByteRun1 run at the end of the row. The original
  wrote the whole run, into the next row or, in the last row, behind the
  plane ([#776](https://github.com/fubar-coder/bb2-c/issues/776)).
* `UnpackIFF` refuses a picture whose header gives 0 planes and returns
  0. The original took it as 256 planes and overwrote its own code
  ([#777](https://github.com/fubar-coder/bb2-c/issues/777)).
* `UnpackIFF`, `LoadIFF` and `ILBMGrab` unpack pictures saved with a
  mask, such as brushes of DPaint, correctly. The original took the mask
  row of every line for the next plane and shifted every line from the
  second on ([#1112](https://github.com/fubar-coder/bb2-c/issues/1112)).
* `ILBMPalette` with an offset on a palette that has no colours yet
  makes it as large as offset and CMAP together. The original made it as
  large as the CMAP and still wrote the colours at the offset, from an
  offset of 2 on behind the palette's memory
  ([#991](https://github.com/fubar-coder/bb2-c/issues/991)).
* `ILBMPalette` refuses a CMAP chunk without a whole colour and returns
  0, and it sizes the palette from the whole CMAP. The original wrote
  65536 colours for such a CMAP, and took the size of a CMAP of 64 KB or
  more from its lower 16 bits
  ([#1105](https://github.com/fubar-coder/bb2-c/issues/1105)).
* `PackPBM` returns -1 for data that does not pack smaller; the
  destination holds the bytes packed up to that point. The original then
  went on to unpack whatever lay at the start of memory into the
  destination
  ([#772](https://github.com/fubar-coder/bb2-c/issues/772)).
* `UnPackPBM` writes no byte past `destlength` and reads none past the
  end of the packed data. The original wrote one byte past the
  destination's length and read one byte behind the data
  ([#778](https://github.com/fubar-coder/bb2-c/issues/778)).
* `DeIce` with data packed with the picture option of Pack-Ice converts
  the number of groups of 8 bytes that the data give, at most the
  unpacked data. The original converted at least 15361 groups and so
  wrote over 120 KB of memory below the end of the unpacked data
  ([#767](https://github.com/fubar-coder/bb2-c/issues/767)).

{{% fixed-bugs-library part="elsewhere" %}}
