---
title: rianimlib
weight: 6
description: The bugs of Blitz Basic 2 in the RWE userlib rianimlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rianimlib {#rianimlib-fixes}

These fixes concern
[rianimlib]({{< relref "/docs/libraries/userlibs/rianimlib" >}}), the
library by Reflective Images that plays IFF animations (ANIM files) of
the delta formats ANIM5 and ANIM7 from memory into bitmaps.
`RIAnimInit` unpacks the first frame and starts the animation, and each
`RINextAnimFrame` draws the next frame. The fix of
[data that is not a `FORM ANIM`](#more-fixes) is listed under
Other fixes.

* `RIAnimInit` takes the bitmap header (BMHD) from the first frame only
  and fails when that frame has none. The original searched the whole
  animation and could take a header from a later part
  ([#511](https://github.com/fubar-coder/bb2-c/issues/511)).
* A failed `RIAnimInit` leaves no animation: `RIAnimFrameCount` and
  `RINextAnimFrame` return 0. The original went on playing the
  animation started before
  ([#512](https://github.com/fubar-coder/bb2-c/issues/512)).
* `RIAnimInit` returns 0 when the bitmap cannot hold the first frame,
  because it is too narrow or not deep enough. The original reported
  success and played the frames into that bitmap
  ([#1106](https://github.com/fubar-coder/bb2-c/issues/1106)).
* `RIAnimInit` refuses an animation whose header gives a depth of 0 or
  more than 8 bitplanes. The original wrote through plane pointers from
  behind the bitmap's
  ([#509](https://github.com/fubar-coder/bb2-c/issues/509)).
* With runtime error checking, `RIAnimInit` with a position `x`,`y`
  outside the bitmap stops with the runtime error "Animation position
  outside the bitmap". The original drew the frames far outside the
  bitmap, as the port does without error checking
  ([#514](https://github.com/fubar-coder/bb2-c/issues/514)).
* `RIAnimFrameCount` counts only the frames. The original counted every
  chunk of the animation, an annotation as well
  ([#510](https://github.com/fubar-coder/bb2-c/issues/510)).
* The library finds the bitmap header, the frames, their colours and
  their delta data behind a chunk of odd length, such as an annotation
  (ANNO); it skips the pad byte that the IFF standard puts there. The
  original read one byte off and missed them
  ([#505](https://github.com/fubar-coder/bb2-c/issues/505)).
* A corrupt chunk length ends the walk through the animation instead of
  sending it into memory that is not there; `RIAnimInit` could hang on
  it in the original
  ([#506](https://github.com/fubar-coder/bb2-c/issues/506)).
* `RIAnimInit` returns 0 for an animation written without pad bytes, in
  which a chunk behind one of odd length starts at an odd address, and
  `RIAnimDepth`, `RIAnimWidth` and `RIAnimHeight` return 0 when this
  happens before the bitmap header. On a 68000 the original crashed with
  a Software Failure on such a file; a 68020 or later read it
  ([#1119](https://github.com/fubar-coder/bb2-c/issues/1119)).
* `RINextAnimFrame` plays a frame that has another chunk, such as a
  DPAN, before its delta data, and returns 0 for a frame without delta
  data. The original crashed there
  ([#515](https://github.com/fubar-coder/bb2-c/issues/515)).
* `RINextAnimFrame` plays ANIM7 animations stored with short (word)
  data. The original decoded every ANIM7 as long data and drew garbage
  ([#504](https://github.com/fubar-coder/bb2-c/issues/504)).
* `RINextAnimFrame` decodes the last column of ANIM5 animations whose
  width is no multiple of 16 pixels. The original left the right edge
  of such frames as it was, and for an animation narrower than 8 pixels
  wrote far beyond the bitmap
  ([#507](https://github.com/fubar-coder/bb2-c/issues/507)).
* `RINextAnimFrame` decodes ANIM7 animations with long data at every
  width. The original left the right edge of frames whose width is no
  multiple of 32 pixels as it was, and for an animation narrower than 32
  pixels wrote far beyond the bitmap. Only the last 16 pixels of a row
  with an odd number of words, such as a width of 33 to 48, are still
  not updated by such frames, since the format does not say what the
  data holds there
  ([#1115](https://github.com/fubar-coder/bb2-c/issues/1115)).
* `RINextAnimFrame` plays on past a chunk between two frames, such as
  an annotation. The original took it for the end of the animation and
  stopped there or started again
  ([#1116](https://github.com/fubar-coder/bb2-c/issues/1116)).
* `RINextAnimFrame` refuses a frame whose header or colour chunk claims
  to be longer than the frame, and returns 0. The original read the
  next chunk from wherever that length pointed in memory
  ([#1117](https://github.com/fubar-coder/bb2-c/issues/1117)).
* `RINextAnimFrame` refuses a frame whose delta data gives a plane's
  data outside the delta chunk, and returns 0 without drawing. The
  original read the delta data of that plane from anywhere in memory
  and wrote it into the bitmap. The runs and copies inside a plane's
  columns are still not checked, so that playing stays as fast as in
  the original
  ([#1120](https://github.com/fubar-coder/bb2-c/issues/1120)).
* `RINextAnimFrame` refuses an ANIM7 frame whose delta data puts the
  data of a plane at an odd offset in the delta chunk, and returns 0
  without drawing. On a 68000 the original crashed with a Software
  Failure on such a frame; the port refuses it on every processor
  ([#1121](https://github.com/fubar-coder/bb2-c/issues/1121)).
* With runtime error checking, `RINextAnimFrame` stops with the runtime
  error "Animation position on an odd byte for ANIM7" before an ANIM7
  frame when the position given to `RIAnimInit` puts the frame at an odd
  byte: an `x` of 8 to 15, 24 to 31 and so on, or an odd byte offset.
  On a 68000 the original crashed with a Software Failure there. ANIM5
  frames play at any position, and without error checking the port
  plays as the original
  ([#1122](https://github.com/fubar-coder/bb2-c/issues/1122)).
* A program run a second time from the editor starts with no animation
  and with `AnimLoop` on. The original's `RIAnimFrameCount` and
  `RINextAnimFrame` worked on the animation of the run before, which may
  have been freed, and an `AnimLoop Off` stayed off
  ([#1107](https://github.com/fubar-coder/bb2-c/issues/1107)).

### More fixes {#more-fixes}

Each point says first what the original did and then what the port
does.

* `RIAnimInit`, `RIAnimDepth`, `RIAnimWidth` and `RIAnimHeight` crashed on
  some data that is not a `FORM ANIM`, such as an IFF `LIST` of type
  `ANIM`; they return normally, and for such a `LIST` `RIAnimDepth` returns
  0 ([#513](https://github.com/fubar-coder/bb2-c/issues/513)).

{{% fixed-bugs-library part="elsewhere" %}}
