---
title: richunkylib
weight: 11
description: The bugs of Blitz Basic 2 in the RWE userlib richunkylib that the BB2 C port fixes.
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

* `ChunkyToPlanar` converts chunky bitmaps of other sizes, with a width
  that is a multiple of 16 pixels, into the top left of any bitmap of 8
  bitplanes that is at least as big, also an interleaved one. A chunky bitmap the
  bitmap cannot take is not converted; with runtime error checking it
  is a runtime error. The original always converted 320 x 256 pixels
  into bitplanes 10240 bytes apart: it read behind a smaller chunky
  bitmap and wrote behind a smaller bitmap
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#chunkytoplanar-sizes" >}}),
  [#884](https://github.com/fubar-coder/bb2-c/issues/884)).
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
* `RaycastSky` with a height of 0 or below copies nothing. The
  original copied 65536 rows, about 20 MB of memory
  ([#887](https://github.com/fubar-coder/bb2-c/issues/887)).
* With runtime error checking, `RaycastSky` stops with "RaycastSky
  outside the chunky bitmap" when the column it would write leaves the
  chunky bitmap, and with "RaycastSky column below 0" for a negative
  texture column. The original wrote or read outside
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#raycastsky-outside" >}}),
  [#887](https://github.com/fubar-coder/bb2-c/issues/887)).
* Freeing a chunky bitmap, when `ChunkyBitmap` makes it again or at the
  end of the program, leaves the audio filter and the power LED as they
  are. The original switched them over
  ([#886](https://github.com/fubar-coder/bb2-c/issues/886)).

{{% fixed-bugs-library part="elsewhere" %}}
