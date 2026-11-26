---
title: rifxlib
weight: 24
description: The bugs of Blitz Basic 2 in the RWE userlib rifxlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/rifxlib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rifxlib {#rifxlib-fixes}

These fixes concern
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}), the bitmap
effects by Reflective Images. The fix of
[the result of `Derez`]({{< relref "/docs/compared-with-original/libraries#rwe-effect-results" >}}) is described for several libraries
together.

* `ClearBitmap` clears the whole area. The original left the last word
  of each bitplane's area as it was and cleared the word in front of
  the area instead, for an `offset` of 0 the word in front of the
  bitplane ([#668](https://github.com/fubar-coder/bb2-c/issues/668)).
* `ClearBitmap`, `FadeInBitmap` and `ZoomyZoom` read the depth of the
  bitmap alone. The original read it together with the bitmap's flags,
  so a bitmap whose flags are set counted as one with hundreds of
  bitplanes and the program crashed
  ([#670](https://github.com/fubar-coder/bb2-c/issues/670)).
* With runtime error checking, every command that works through the
  bitplanes of a bitmap stops with "Illegal BitMap Depth" for a bitmap
  of depth 0 or more than 8, `ClearBitmap` and `FadeInBitmap` with
  "Delay must not be 0" for a delay of 0, and `ZoomyZoom` with "ZoomyZoom
  needs a bitmap of at least 640 x 32" for a smaller one
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#rwe-effects-depth" >}}),
  [#947](https://github.com/fubar-coder/bb2-c/issues/947),
  [#674](https://github.com/fubar-coder/bb2-c/issues/674),
  [#669](https://github.com/fubar-coder/bb2-c/issues/669),
  [#676](https://github.com/fubar-coder/bb2-c/issues/676)).
* After a `Derez` or `ZoomXY` with a bitmap of depth 0, the next calls
  count the bitplanes of their own bitmaps. In the original the count
  stayed wrong for every later call
  ([#675](https://github.com/fubar-coder/bb2-c/issues/675)).
* `ZoomXY` works after a `Derez` with a value of 3 or more. In the
  original, such a `Derez` wrote its table of masks over the first
  instructions of `ZoomXY`: after a `Derez` of 5, `ZoomXY` cleared a
  byte somewhere in memory and zoomed nothing
  ([#671](https://github.com/fubar-coder/bb2-c/issues/671)).
* With runtime error checking, `ZoomXY` before any `InitZoomXY` stops
  with "ZoomXY needs InitZoomXY". The original zoomed a bitmap at
  address 0 and crashed
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#zoomxy-without-initzoomxy" >}}),
  [#1131](https://github.com/fubar-coder/bb2-c/issues/1131)).
* A program run a second time from the editor starts without the
  bitmaps and offsets of an `InitZoomXY`. In the original, a `ZoomXY`
  before the run's own `InitZoomXY` zoomed with the bitmaps of the run
  before, which may have been freed
  ([#1127](https://github.com/fubar-coder/bb2-c/issues/1127)).

{{% fixed-bugs-library part="elsewhere" %}}
