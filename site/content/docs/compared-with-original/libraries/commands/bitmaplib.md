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

* `BitMap` with a depth of 0 or more than 8 stopped, with runtime error
  checking, with the error text "No currently used object!", or an empty
  text in an executable; the text is "Illegal BitMap Depth", as for
  `CludgeBitMap`
  ([#1040](https://github.com/fubar-coder/bb2-c/issues/1040)).
* A `BitMap` or `CludgeBitMap` of width or height 0 was accepted, and
  `CopyBitMap` of it copied about four thousand million words through the
  memory; with runtime error checking a width or height below 1 is the
  error "Illegal BitMap Size"
  ([#78](https://github.com/fubar-coder/bb2-c/issues/78)).
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
* `CludgeBitMap` with a depth of 0 wrote 65536 plane pointers behind the
  bitmap object, and `InitShape` made a shape of depth 0. The drawing and
  blitting commands, such as `Plot`, `Polyf`, `BBlit` and `QBlit`, then
  ran up to 65536 times over memory behind the object. With runtime error
  checking, `CludgeBitMap` refuses a depth of 0 or more than 8 with
  "Illegal BitMap Depth", as `BitMap` does, and `InitShape` a depth of 0
  with "Illegal Shape Depth"
  ([#393](https://github.com/fubar-coder/bb2-c/issues/393),
  [#74](https://github.com/fubar-coder/bb2-c/issues/74),
  [#243](https://github.com/fubar-coder/bb2-c/issues/243)).

### IFF files

These are the commands that load and save IFF files, the Amiga's standard
format for pictures (ILBM), colours, sounds and animations.

* `LoadBitMap` and `LoadScreen` of a file that could not be opened, or of
  one without a picture header (BMHD), went on with what an earlier load
  had left in the read buffer: they showed the old picture again or
  garbage, or stopped with a message that did not fit. They stop with
  "Can't Open File" or "Can't Find IFF 'BMHD'"
  ([#187](https://github.com/fubar-coder/bb2-c/issues/187);
  [`LoadBitMap` and `LoadScreen` of a missing file]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#loadbitmap-missing-file" >}})).
* `LoadBitMap` and `LoadScreen` of a file without picture data stopped
  with "Can't Find IFF 'BODY'" and left the file open, so it stayed
  locked until the next reboot, also when the program went on with
  `SetErr`; the file is closed before the error
  ([#1053](https://github.com/fubar-coder/bb2-c/issues/1053)).
* `LoadBitMap`, `LoadScreen` and `LoadShape` of a file that ends inside
  its picture data stopped with "Can't Read File" and left the file open
  and locked until the next reboot; the file is closed before the error
  ([#1065](https://github.com/fubar-coder/bb2-c/issues/1065)).

{{% fixed-bugs-library part="elsewhere" %}}
