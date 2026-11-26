---
title: rifxlib
weight: 24
description: The bugs of Blitz Basic 2 in the RWE userlib rifxlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/rifxlib/]
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `Derez` returned no useful value {#derez-result}

`Derez` of
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}) makes a
picture blocky. It can also be used as a function: it returns -1 when it
did its work and 0 when it did nothing, for a `derez_value` of 0.

**What happened.** The original set no result: a program that used
`Derez` as a function got whatever was left in the register: a value
of the program from before the call, a loop counter, or a word of a
table or of the picture. This program asks `Derez` for its result four times:

```
DEFTYPE .l
BitMap 1,320,16,1
BitMap 2,320,16,1
r=Derez(1,2,0,0,0,8) : NPrint "0: ",r
r=Derez(1,2,0,0,1,8) : NPrint "1: ",r
r=Derez(1,2,0,0,2,8) : NPrint "2: ",r
r=Derez(1,2,0,0,3,8) : NPrint "3: ",r
```

With the original it printed `64`, `65535`, `0` and `0`; with the port
it prints `0`, `-1`, `-1` and `-1`.

**What to change.** A program that uses `Derez` as a statement works as
before.

Issue: [Derez is declared with a long result but returns no defined value (#672)](https://github.com/fubar-coder/bb2-c/issues/672).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rifxlib {#rifxlib-fixes}

These fixes concern
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}), the bitmap
effects by Reflective Images. The fix of
[the result of `Derez`](#derez-result) is described above.

* `ClearBitmap` clears the whole area. The original left the last word
  of each bitplane's area as it was and cleared the word in front of
  the area instead, for an `offset` of 0 the word in front of the
  bitplane ([#668](https://github.com/fubar-coder/bb2-c/issues/668)).
* `ClearBitmap`, `FadeInBitmap` and `ZoomyZoom` read the depth of the
  bitmap alone. The original read it together with the bitmap's flags,
  so a bitmap whose flags are set counted as one with hundreds of
  bitplanes and the program crashed
  ([#670](https://github.com/fubar-coder/bb2-c/issues/670)).
* After a `Derez` or `ZoomXY` with a bitmap of depth 0, the next calls
  count the bitplanes of their own bitmaps. In the original the count
  stayed wrong for every later call
  ([#675](https://github.com/fubar-coder/bb2-c/issues/675)).
* `ZoomXY` works after a `Derez` with a value of 3 or more. In the
  original, such a `Derez` wrote its table of masks over the first
  instructions of `ZoomXY`: after a `Derez` of 5, `ZoomXY` cleared a
  byte somewhere in memory and zoomed nothing
  ([#671](https://github.com/fubar-coder/bb2-c/issues/671)).
* A program run a second time from the editor starts without the
  bitmaps and offsets of an `InitZoomXY`. In the original, a `ZoomXY`
  before the run's own `InitZoomXY` zoomed with the bitmaps of the run
  before, which may have been freed
  ([#1127](https://github.com/fubar-coder/bb2-c/issues/1127)).

#### Bitmap depths for the effects of rifxlib and `CPUCls` {#rwe-effects-depth}

The effects of
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}), such as
`ClearBitmap`, `FadeInBitmap`, the zooms and `Derez`, and `CPUCls` of
[rigfxlib]({{< relref "/docs/libraries/userlibs/rigfxlib" >}}) work
through the bitplanes of a bitmap object one by one. The guide of
rifxlib allows at most 8 bitplanes.

**What happened.** The original checked the depth nowhere, also not
with runtime error checking. A bitmap of depth 0 ran their loops 65536
or 256 times, over the plane pointers and the memory behind the bitmap
object and over the library's own data. With more bitplanes than its
tables hold, rifxlib wrote its plane pointers over its own variables
and, from 11 or 21 planes on, over its code; `CPUCls` cleared memory through the
pointers behind the bitmap's eight.

**With the port.** With runtime error checking, a bitmap of depth 0 or
more than 8 is the error "Illegal BitMap Depth". For `ZoomXY` the check is on the bitmaps chosen
with `InitZoomXY`. Without runtime error checking, the commands are as
before.

**What to change.** Use bitmaps of 1 to 8 bitplanes.

Issues:

* [Derez and ZoomXY with a depth-0 source bitmap count 65536 planes (#947)](https://github.com/fubar-coder/bb2-c/issues/947)
* [The 16-entry plane pointer arrays are not checked against the depth: FadeInBitmap overflows with more than 8 planes (#674)](https://github.com/fubar-coder/bb2-c/issues/674)
* [CPUCls: depth 0 clears 65536 planes (#679)](https://github.com/fubar-coder/bb2-c/issues/679)

#### A delay of 0 for `ClearBitmap` and `FadeInBitmap` {#rifxlib-delay-zero}

`ClearBitmap` and `FadeInBitmap` of
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}) wait after
every line for a number of turns of an empty loop, the delay, so that
the effect can be seen.

**What happened.** The loop counts down before it tests, so a delay of
0 meant 4294967296 turns per line, and the program seemed to hang. The
library's guide warns against 0, but the original did not check it,
also not with runtime error checking.

**With the port.** With runtime error checking, a delay of 0 is the
error "Delay must not be 0". Without runtime error checking, the
commands are as before.

**What to change.** Give a delay of 1 or more.

Issue: [ClearBitmap/FadeInBitmap: delay 0 means 2^32 turns of the busy loop per line (#669)](https://github.com/fubar-coder/bb2-c/issues/669).

#### `ZoomyZoom` on a bitmap smaller than 640 x 32 {#zoomyzoom-size}

`ZoomyZoom` of
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}) doubles
the left half of the first 32 lines of a bitmap 640 pixels wide in
place.

**What happened.** The original did not look at the size of the bitmap,
also not with runtime error checking: it always worked on 32 lines of
80 bytes. On a narrower bitmap these were not its lines, and on a
smaller one it wrote behind the bitplanes.

**With the port.** With runtime error checking, a bitmap less than 640
pixels wide or less than 32 lines high is the error "ZoomyZoom needs a
bitmap of at least 640 x 32". Without runtime error checking, the
command is as before.

**What to change.** Use `ZoomyZoom` only on bitmaps of at least 640 x
32 pixels.

Issue: [ZoomyZoom ignores the bitmap's size: always 32 lines of 80 bytes (#676)](https://github.com/fubar-coder/bb2-c/issues/676).

#### `ZoomXY` without `InitZoomXY` {#zoomxy-without-initzoomxy}

`ZoomXY` of
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}) takes no
bitmap of its own: it zooms from the source into the destination that
`InitZoomXY` chose, so `InitZoomXY` must come first.

**What happened.** The original did not look whether `InitZoomXY` had
chosen any bitmaps, also not with runtime error checking. Before it,
`ZoomXY` read a bitmap at address 0, took the processor's exception
vectors there for its bitplane pointers and wrote through them, and the
machine crashed.

**With the port.** With runtime error checking, `ZoomXY` before any
`InitZoomXY` of the run is the error "ZoomXY needs InitZoomXY". Without
runtime error checking, the command works as in the original.

**What to change.** Call `InitZoomXY` before the first `ZoomXY`.

Issue: [ZoomXY before InitZoomXY reads a bitmap at address 0 and writes through the exception vectors (#1131)](https://github.com/fubar-coder/bb2-c/issues/1131).

{{% fixed-bugs-library part="elsewhere" %}}
