---
title: bitmaplib
weight: 9
description: The bugs of Blitz Basic 2 in the command library bitmaplib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/bitmaplib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Bitmaps

* A bitmap whose height times depth is 65536 or more, such as
  `BitMap 0,16,8192,8`, got too little memory, and its planes pointed
  behind it into memory of others; it gets all of its memory
  ([#76](https://github.com/fubar-coder/bb2-c/issues/76)).
* `ShapesBitMap` of a shape with planes of 32 KB or more put each further
  plane of the bitmap in front of the previous one, so drawing on it wrote
  in front of the shape; the planes follow each other as in the shape
  ([#79](https://github.com/fubar-coder/bb2-c/issues/79)).
* `BitMap` set only the plane pointers up to its depth and left the
  others of the object as they were, pointing into the freed memory of an
  earlier bitmap of more planes; they are cleared
  ([#80](https://github.com/fubar-coder/bb2-c/issues/80)).
* `LoadBitMap` and `LoadScreen` of a picture with more planes than a
  bitmap whose flags are set wrote the extra planes through the memory
  behind the bitmap; they load only the planes the bitmap has
  ([#188](https://github.com/fubar-coder/bb2-c/issues/188)).

#### `BitMap` with a wrong depth {#bitmap-depth-text}

**What happened.** With runtime error checking, `BitMap` with a depth of
0 or more than 8 stopped with the error text "No currently used object!",
which has nothing to do with the depth; in an executable the text was
empty.

**With the port.** The error text is "Illegal BitMap Depth", as for
`CludgeBitMap`.

**What to change.** Give a depth from 1 to 8.

Issue: [BitMap with a wrong depth reported "No currently used object!" (#1040)](https://github.com/fubar-coder/bb2-c/issues/1040).

#### A bitmap of width or height 0 {#bitmap-size}

**What happened.** `BitMap` and `CludgeBitMap` accepted a width or
height of 0, also with runtime error checking. `CopyBitMap` of such a
bitmap copied about four thousand million words through the memory.

```
BitMap 0,16,10,1
NPrint "16 x 10"
BitMap 1,0,10,1
NPrint "0 x 10"
End
```

Compiled with runtime error checking, the original printed both lines.

**With the port.** With runtime error checking, a width or height below 1
is the error "Illegal BitMap Size"; the program above stops after
`16 x 10`.

**What to change.** Give a width and a height of 1 or more.

Issue: [CopyBitMap of a bitmap of size 0 copied 4 billion words (#78)](https://github.com/fubar-coder/bb2-c/issues/78).

#### A bitmap or shape of depth 0 {#depth-zero}

**What happened.** `BitMap` refuses a depth of 0 with runtime error
checking, but `CludgeBitMap` and `InitShape` had no check. A depth of 0
made a bitmap or shape that the drawing and blitting commands, such as
`Plot`, `Polyf`, `BBlit` and `QBlit`, treated as one of 65536 planes, so
they ran over the memory behind it; `CludgeBitMap` itself already wrote
65536 plane pointers behind the bitmap object.

```
DEFTYPE .l
NPrint "before"
InitShape 0,16,8,0
NPrint "after, depth ",Peek.w(Addr Shape(0)+4)
End
```

Compiled with runtime error checking, the original printed

```
before
after, depth 0
```

**With the port.** With runtime error checking, `CludgeBitMap` with a
depth of 0 or more than 8 is the error "Illegal BitMap Depth", and
`InitShape` with a depth of 0 the error "Illegal Shape Depth". The
program above stops after `before`. The check of `InitShape` also keeps
such a shape out of `ShapeGadget`.

**What to change.** Give a depth from 1 to 8.

Issues:

* [Bitmaps of depth 0 made the plane loops run 65536 times (#393)](https://github.com/fubar-coder/bb2-c/issues/393)
* [BBlit with depth 0 (#74)](https://github.com/fubar-coder/bb2-c/issues/74)
* [QBlit with a shape of depth 0 (#243)](https://github.com/fubar-coder/bb2-c/issues/243)
* [A shape gadget of depth 0 (#167)](https://github.com/fubar-coder/bb2-c/issues/167)

### IFF files

These are the commands that load and save IFF files, the Amiga's standard
format for pictures (ILBM), colours, sounds and animations.

* `LoadBitMap` and `LoadScreen` of a file without picture data stopped
  with "Can't Find IFF 'BODY'" and left the file open, so it stayed
  locked until the next reboot, also when the program went on with
  `SetErr`; the file is closed before the error
  ([#1053](https://github.com/fubar-coder/bb2-c/issues/1053)).
* `LoadBitMap`, `LoadScreen` and `LoadShape` of a file that ends inside
  its picture data stopped with "Can't Read File" and left the file open
  and locked until the next reboot; the file is closed before the error
  ([#1065](https://github.com/fubar-coder/bb2-c/issues/1065)).

#### `LoadBitMap` and `LoadScreen` of a missing file {#loadbitmap-missing-file}

**What happened.** `LoadBitMap` and `LoadScreen` without a palette did
not notice that the file could not be opened, or that it had no picture
header (BMHD). They looked for the picture data in what an earlier IFF
load had left in the read buffer, and showed the old picture again,
garbage, or stopped with "Can't Find IFF 'BODY'".

**With the port.** A file that cannot be opened is the runtime error
"Can't Open File", a file without a picture header the error "Can't Find
IFF 'BMHD'", also without runtime error checking. With a palette, the
colours are read first, and a file that cannot be opened is the error
"Can't Open IFF", as before.

**What to change.** Where the file may be missing, test with `Exists`
that it is there before it is loaded.

Issue: [LoadBitMap ignored a file it could not open (#187)](https://github.com/fubar-coder/bb2-c/issues/187).

{{% fixed-bugs-library part="elsewhere" %}}
