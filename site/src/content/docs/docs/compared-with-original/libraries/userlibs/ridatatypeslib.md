---
title: ridatatypeslib
description: The bugs of Blitz Basic 2 in the RWE userlib ridatatypeslib that
  the BB2 C port fixes.
sidebar:
  order: 15
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/ridatatypeslib/
---

::fixed-bugs-library

## Changes for existing programs

### A second `DataTypeCreate` lost the object before {#datatypecreate-second}

[ridatatypeslib](/docs/libraries/userlibs/ridatatypeslib/)
loads files through the datatypes of Workbench 3. It works on one file
and one object at a time: `DataTypeOpen` opens the file,
`DataTypeCreate` loads it into a datatype object, and `DataTypeClose`
frees the object and closes the file. `DataTypeOpen` and
`AttachDataType` already refuse a second file or window while the first
is in use.

**What happened.** A second `DataTypeCreate` made a new object and
stored it over the first, which nothing freed any more: the memory of
the first object, for a picture its whole bitmap, was lost until the
system was reset. This program loads a picture of 320 x 200 pixels with
16 colours, calls `DataTypeCreate` twice and then tests whether more
than 16000 bytes of memory are missing after `DataTypeClose`. A first
round loads the system's classes, so that they do not count:

```
DEFTYPE .l
If DataTypeOpen("CORPUS:p.ilbm")
  o=DataTypeCreate
  DataTypeClose
EndIf
m0=AvailMem_(0)
If DataTypeOpen("CORPUS:p.ilbm")
  o=DataTypeCreate
  NPrint "first ",o<>0
  o=DataTypeCreate
  NPrint "second ",o<>0
  DataTypeClose
EndIf
m1=AvailMem_(0)
NPrint "lost ",m0-m1>16000
End
```

**With the original**

```
first -1
second -1
lost -1
```

**With the port**

```
first -1
second 0
lost 0
```

With the port, `DataTypeCreate` returns 0 while an object exists, and
the first object stays in use until `DataTypeClose`.

**What to change.** A program that called `DataTypeCreate` once per
`DataTypeOpen` works as before. One that called it again to load the
file anew, for example with another tag list, gets 0 and keeps the
first object: call `DataTypeClose` and `DataTypeOpen` before the next
`DataTypeCreate`.

Issue: [A second DataTypeCreate loses the object made before (#600)](https://github.com/fubar-coder/bb2-c/issues/600).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ridatatypeslib {#ridatatypeslib-fixes}

These fixes concern
[ridatatypeslib](/docs/libraries/userlibs/ridatatypeslib/),
the datatypes of Workbench 3 for Blitz programs, by Steve Matty for
Red When Excited.
`DataTypeOpen` opens a file, `DataTypeCreate` loads it into a datatype
object, `DataTypeBitmap` reads the size and depth of a picture, and
`AttachDataType` shows the object in a window. The fix of
[a second `DataTypeCreate`](#datatypecreate-second) is described above.

* `DataTypeOpen` of a file for which no datatype is found returns 0 and
  unlocks the file again. The original read a nonsense datatype and kept
  the file locked, even after `DataTypeClose`, so every later
  `DataTypeOpen` of the program failed
  ([#605](https://github.com/fubar-coder/bb2-c/issues/605)).
* `DataTypeOpen` takes a file name of any length. The original copied
  it into a buffer of 384 bytes without a limit, so a longer name, even
  of a file that does not exist, overwrote the library's data
  ([#604](https://github.com/fubar-coder/bb2-c/issues/604)).
* `DataTypeCreate` without a tag list returns 0 when no file is open,
  as the form with a tag list does. The original made an object of the
  file opened last, which was closed since
  ([#601](https://github.com/fubar-coder/bb2-c/issues/601)).
* `DataTypeClose` removes an object that is still attached from its
  window before it frees it. The original freed it in the window, which
  then used freed memory at its next refresh, click or close
  ([#599](https://github.com/fubar-coder/bb2-c/issues/599)).
* `DataTypeBitmap` of an object that is not a picture returns 0. The
  original returned a value that the command before had left in the
  library's buffer, and `DataTypeWidth` and the other size functions
  read through it
  ([#598](https://github.com/fubar-coder/bb2-c/issues/598)).
* `AttachDataType window,id` without `DataTypeBitmap` before reads the
  picture's size itself, so the object gets the size of the picture; an
  object that is not a picture is not attached, and the result is 0. The
  original attached it at a size of 0 x 0
  ([#595](https://github.com/fubar-coder/bb2-c/issues/595)).
* `AttachDataType window,id,&taglist` passes the program's tag list on
  to the object. The original ignored it and took the id for the address
  of a tag list
  ([#596](https://github.com/fubar-coder/bb2-c/issues/596)).
* `AttachDataType window,id,x,y,width,height` passes the position and
  the size as given, negative values too. The original could pass huge
  values made of what other code had left in the registers: a top of 3
  reached the system as 1073938435
  ([#597](https://github.com/fubar-coder/bb2-c/issues/597)).
* `AttachDataType` with a position or a tag list returns 0 when there is
  no object, as the form without them does. The original passed the
  object 0 to the system and returned -1, and then refused every
  further `AttachDataType` until `DetachDataType`
  ([#1098](https://github.com/fubar-coder/bb2-c/issues/1098)).
* `DataTypeModeID` with a tag list uses the program's tags after its
  own three, and leaves the program's list unchanged. The original wrote
  its tags over the start of the program's list and so ignored the
  program's tags
  ([#603](https://github.com/fubar-coder/bb2-c/issues/603)).
* `DataTypesPalette` copies the picture's colours, at most as many as
  the palette object has. The original copied as many as the palette
  has: a palette larger than the picture's got the bytes behind the
  picture's colours as further colours, and a palette of 0 colours made
  it write 786432 bytes behind the palette's data
  ([#606](https://github.com/fubar-coder/bb2-c/issues/606)).
* `DataTypesPalette` returns 0, and copies nothing, for an object that
  has no colours. The original could report success then, and for an
  object that is not a picture it copied colours from an address left
  by the command before
  ([#607](https://github.com/fubar-coder/bb2-c/issues/607)).
* Without datatypes.library, a program that uses the library still
  closes dos.library, intuition.library and graphics.library at its end;
  the original left them open, once more with every run
  ([#602](https://github.com/fubar-coder/bb2-c/issues/602)).

::fixed-bugs-library{part="elsewhere"}
