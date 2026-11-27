---
title: rianimlib
description: The bugs of Blitz Basic 2 in the RWE userlib rianimlib that the BB2
  C port fixes.
sidebar:
  order: 6
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/rianimlib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rianimlib {#rianimlib-fixes}

These fixes concern
[rianimlib](/docs/libraries/userlibs/rianimlib/), the
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
* A program run a second time from the editor starts with no animation
  and with `AnimLoop` on. The original's `RIAnimFrameCount` and
  `RINextAnimFrame` worked on the animation of the run before, which may
  have been freed, and an `AnimLoop Off` stayed off
  ([#1107](https://github.com/fubar-coder/bb2-c/issues/1107)).

#### An animation position outside the bitmap {#anim-position-outside}

`RIAnimInit` of
[rianimlib](/docs/libraries/userlibs/rianimlib/) unpacks
the first frame of an animation into a bitmap and takes the position of
the animation in the bitmap either as a pixel position `x`,`y`, with `x`
rounded down to a multiple of 8, or as a byte offset into the
bitplanes. `RINextAnimFrame` then draws each frame at that position.

**What happened.** The original turned `x`,`y` into a byte offset
without a look at the bitmap, also not with runtime error checking. A
position outside the bitmap put the frames into the memory behind it,
and a negative `y` far away from it. With an animation loaded at address
`a`, this program printed `before` and `after -1`, and the first frame
went into the row below the one the position named:

```
BitMap 0,32,16,1
NPrint "before"
r=RIAnimInit(a,0,0,32,0)
NPrint "after ",r
```

**With the port.** With runtime error checking, an `x` outside 0 to
the bitmap's width minus 1 or a `y` outside 0 to its height minus 1 is
the error "Animation position outside the bitmap"; the program above
prints `before` and stops at `RIAnimInit`. The byte offset form is not
checked. Without runtime error checking, the command works as in the original.

**What to change.** Give a position inside the bitmap, and make the
bitmap large enough for the whole animation at that position: that part
is not checked.

Issue: [RIAnimInit x,y: the row offset is MULU.w of the low words – a negative y gives a huge offset (#514)](https://github.com/fubar-coder/bb2-c/issues/514).

#### An ANIM7 animation at an odd byte {#anim7-odd-byte}

`RIAnimInit` places an animation at a byte of the bitmap
([An animation position outside the bitmap](#anim-position-outside)).

**What happened.** The decoder of the ANIM7 format writes whole words
and longs into the bitplanes, which a 68000 can only do at an even
address. The original did not check the position, also not with runtime
error checking: at an odd byte, an `x` of 8 to 15, 24 to 31 and so on or
an odd byte offset, the first ANIM7 frame crashed a 68000 with a Software
Failure. With an ANIM7 animation loaded at address `m`, this crashed at
`RINextAnimFrame`:

```
BitMap 0,64,16,1
If RIAnimInit(m,0,0,8,0)
  r=RINextAnimFrame(0)
EndIf
```

**With the port.** With runtime error checking, `RINextAnimFrame` stops
before an ANIM7 frame at an odd byte with the error "Animation position
on an odd byte for ANIM7", on every processor. ANIM5 frames are written
byte by byte and play at any position. Without runtime error checking,
the commands work as in the original.

**What to change.** Place ANIM7 animations at an even byte: an `x` of
0 to 7, 16 to 23 and so on, or an even byte offset.

Issue: [RIAnimInit with an odd byte offset (`x>>3` or `xy_offset`) for an ANIM7 animation: address error on a 68000 at RINextAnimFrame, no error check (#1122)](https://github.com/fubar-coder/bb2-c/issues/1122).

### More fixes {#more-fixes}

Each point says first what the original did and then what the port
does.

* `RIAnimInit`, `RIAnimDepth`, `RIAnimWidth` and `RIAnimHeight` crashed on
  some data that is not a `FORM ANIM`, such as an IFF `LIST` of type
  `ANIM`; they return normally, and for such a `LIST` `RIAnimDepth` returns
  0 ([#513](https://github.com/fubar-coder/bb2-c/issues/513)).

::fixed-bugs-library{part="elsewhere"}
