---
title: rishapeslib
description: The bugs of Blitz Basic 2 in the RWE userlib rishapeslib that the
  BB2 C port fixes.
sidebar:
  order: 39
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/rishapeslib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rishapeslib {#rishapeslib-fixes}

These fixes concern
[rishapeslib](/docs/libraries/userlibs/rishapeslib/),
the shape helpers by Reflective Images with their own shapes file, the
LE shapes file: `LESaveShapes` writes it, `LELoadShapes` loads it from
disk and `LECludgeShapes` decodes one in memory; `CludgeShapes` makes
shapes from memory in the form `SaveShapes` writes. The fix of
[shapes linked by an earlier `LECludgeShapes`](#more-fixes) is
listed under Other fixes.

* `CludgeShapes` takes a shape in memory whose data size is 0 as an
  empty shape. The original copied 64 KB for it and overwrote memory
  ([#808](https://github.com/fubar-coder/bb2-c/issues/808)).
* `CludgeShapes` reads the shapes behind an empty shape in memory from
  the right place. The original skipped the empty shape by 32 bytes
  plus the data size of the shape before it
  ([#809](https://github.com/fubar-coder/bb2-c/issues/809)).
* `LELoadShapes`, `LECludgeShapes` and `LEInitShape` free the cookie of
  the shape they replace, so loading shapes again does not use up chip
  memory ([#810](https://github.com/fubar-coder/bb2-c/issues/810)).
* `LECludgeShapes` finds the shapes of LE files whose header chunk is
  longer than the library's own, or whose palette chunk has a pad byte.
  The original took fixed sizes for both and missed the shape data
  ([#811](https://github.com/fubar-coder/bb2-c/issues/811)).
* `LECludgeShapes` decodes LE files whose palette has an odd number of
  colours, as `LESaveShapes` writes them, on a 68000 too; such shapes
  are copied into chip memory instead of being used where they are. The
  original crashed a 68000 with a Software Failure, and on a 68020 it
  used the shapes at an odd address, where the blitter cannot draw them
  ([#1118](https://github.com/fubar-coder/bb2-c/issues/1118)).
* Loading LE shapes with a palette leaves the palette as it is for a
  palette chunk shorter than one colour. The original wrote 65536
  colours, 768 KB, over memory
  ([#820](https://github.com/fubar-coder/bb2-c/issues/820)).
* `LESaveShapes` writes the palette whenever one is given. The original
  wrote it or not depending on the flags the system's write routine
  left behind ([#814](https://github.com/fubar-coder/bb2-c/issues/814)).
* `LESaveShapes` with a palette of 0 colours writes no palette. The
  original copied 65536 colours and overwrote 192 KB of memory
  ([#823](https://github.com/fubar-coder/bb2-c/issues/823)).
* Files written by `LESaveShapes` give the right length in their IFF
  header, so other IFF readers see the whole file. The original wrote
  a length 8 too large, and after a save with a palette also counted
  that palette in a later save without one
  ([#816](https://github.com/fubar-coder/bb2-c/issues/816)).
* `LESaveShapes` asks for 12 bytes for its shape header. The original
  could ask for up to 4 GB, depending on what a register held before
  ([#817](https://github.com/fubar-coder/bb2-c/issues/817)).
* `LESaveShapes` takes the number of shapes from the lower 16 bits of
  the shape numbers, as they are words. The original used all 32 bits:
  `LESaveShapes 0,a` with a long variable `a` of `$50009`, shape 9 in
  its lower 16 bits, wrote a file that claimed 327689 shapes
  ([#815](https://github.com/fubar-coder/bb2-c/issues/815)).
* A program run a second time from the editor starts with `LEAutoCookie`
  on and the values of `LEMaxShapeWidth`, `LEMaxShapeHeight` and
  `LEMaxShapeDepth` at 0. The original kept those of the run before
  ([#1109](https://github.com/fubar-coder/bb2-c/issues/1109)).

### More fixes {#more-fixes}

Each point says first what the original did and then what the port
does.

* `LELoadShapes` and `LECludgeShapes` into shapes that an earlier
  `LECludgeShapes` had linked into memory read the shape from a wrong
  place and ended early without closing the file, and a second such shape
  crashed the program; such a shape is replaced like any other
  ([#807](https://github.com/fubar-coder/bb2-c/issues/807)).

::fixed-bugs-library{part="elsewhere"}
