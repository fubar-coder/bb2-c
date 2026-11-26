---
title: richunkylib
weight: 11
description: The bugs of Blitz Basic 2 in the RWE userlib richunkylib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/richunkylib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in richunkylib {#richunkylib-fixes}

These fixes concern
[richunkylib]({{< relref "/docs/libraries/userlibs/richunkylib" >}}),
the chunky bitmaps of Reflective Images, with one byte per pixel, and
their conversion into planar bitmaps.

* `ChunkyToPlanar` reads the depth of the bitmap alone. The original
  read it together with the bitmap's flags: depending on the flags it
  wrote 8 bitplanes into a bitmap with fewer, or refused a bitmap of
  depth 8
  ([#883](https://github.com/fubar-coder/bb2-c/issues/883)).
* `ChunkyBitmap` frees the data a chunky bitmap already holds also when
  it lies below address 65536. The original never freed such data, so a
  new chunky bitmap on that object lost it; when the new memory could
  not be allocated, the object kept the old address with a size of 0
  ([#885](https://github.com/fubar-coder/bb2-c/issues/885)).
* Freeing a chunky bitmap, when `ChunkyBitmap` makes it again or at the
  end of the program, leaves the audio filter and the power LED as they
  are. The original switched them over
  ([#886](https://github.com/fubar-coder/bb2-c/issues/886)).

#### `ChunkyToPlanar` with sizes that do not fit {#chunkytoplanar-sizes}

`ChunkyToPlanar` of
[richunkylib]({{< relref "/docs/libraries/userlibs/richunkylib" >}})
converts a chunky bitmap, one byte per pixel, into the eight bitplanes
of a bitmap. It converts the chunky bitmap's width and height into the
top left of the bitmap, row by row.

**What happened.** The original always converted 320 x 256 pixels into
bitplanes 10240 bytes apart, whatever the sizes of the chunky bitmap
and of the bitmap, also with runtime error checking. It read behind a
smaller chunky bitmap, wrote behind a smaller bitmap and mixed up the
bitplanes of an interleaved one.

**With the port.** `ChunkyToPlanar` converts into the top left of any
bitmap of 8 bitplanes that is at least as big, also an interleaved one.
A chunky bitmap whose width is not a multiple of 16,
one that is wider or higher than the bitmap, and a bitmap whose
bitplanes are not equally far apart are not converted: nothing is
written. With runtime error checking these are the errors "Chunky
bitmap width not a multiple of 16", "Chunky bitmap larger than the
bitmap" and "Bitmap planes not equally spaced". A bitmap of fewer than
8 bitplanes and a chunky bitmap without data do nothing, as in the
original, also with runtime error checking.

**What to change.** Give the chunky bitmap a width that is a multiple
of 16, and convert it into a bitmap of 8 bitplanes that is at least as
wide and as high.

Issue: [ChunkyToPlanar ignores the sizes of the chunky bitmap and of the bitmap (always 320×256×8) (#884)](https://github.com/fubar-coder/bb2-c/issues/884).

#### `RaycastSky` outside the chunky bitmap {#raycastsky-outside}

`RaycastSky` of
[richunkylib]({{< relref "/docs/libraries/userlibs/richunkylib" >}})
copies `height` bytes of a column of a texture into a column of a
chunky bitmap, both with rows of 320 bytes. `x` and `column` are
offsets from the start of the chunky bitmap and of the texture.

**What happened.** The original checked neither offset, also not with
runtime error checking. A negative `x` wrote in front of the chunky
bitmap, an `x` whose column runs past the last row wrote behind it, and
a negative `column` read in front of the texture. A height of 0 made
it copy 65536 rows, about 20 MB of memory.

**With the port.** With runtime error checking, an `x` below 0, or one
whose column would run past the end of the chunky bitmap, is the error
"RaycastSky outside the chunky bitmap", and a `column` below 0 is the
error "RaycastSky column below 0". A height below 1 is not checked:
`RaycastSky` copies nothing for it. Without runtime error checking, the
offsets are not checked.

**What to change.** Keep `x` and `column` at 0 or above, and the column
of `height` rows inside the chunky bitmap.

Issue: [RaycastSky with height 0 copies 65536 rows; x and column are not checked (#887)](https://github.com/fubar-coder/bb2-c/issues/887).

{{% fixed-bugs-library part="elsewhere" %}}
