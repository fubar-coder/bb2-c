---
title: The RWE userlibs
weight: 5
description: The libraries of Red When Excited and Reflective Images that the BB2 C port builds as userlibs, which variant they need, and how to use them.
---

Before this page you should know what a library variant is ([Library
variants]({{< relref "/docs/blitz2-internals/variants" >}})).

Besides the libraries of Blitz Basic 2 itself, many Blitz programmers
used libraries by others, most of them by Reflective Images. Red When
Excited released the sources of a collection of such libraries in
September 1999. The BB2 C port translates them to C, like its own
libraries, and builds them as userlibs.

## What they are

The collection covers a wide range: AmigaOS features such as App windows
and icons, commodities, DOS notification, data types, ARexx and the
requesters of reqtools; graphics such as copper effects, chunky buffers,
shapes, displays, bitmap fonts and animations; sound and music players;
data tools such as sorting, linked lists, packing, crunching and
encryption; and helpers for debugging. Each library is a C file whose
first comment says what it provides and where it comes from:
[the RWE userlibs](https://github.com/fubar-coder/bb2-c/tree/main/libs/userlibs/rwe)
on GitHub.

## How they are checked

Each of these libraries is checked against a reference, as the libraries
of Blitz Basic 2 are: its declarations must equal those of the original
library file, and its behaviour is compared with the original in the
emulator. Where the released source is newer than every binary that
exists, the reference is the source itself, compiled with the original
Blitz Basic 2.10 in the emulator.

## Which variant they need

These libraries have numbers that Blitz Basic 2's libraries do not use,
but the numbers are not part of the default variant. They are built for
the variant **rwe-1999**, which has the default numbers plus theirs. Two
libraries need a variant of their own:

* **The expression evaluator** uses a number that Blitz Basic 2 gives to
  one of its own libraries. The variant **rwe-1999-eval** moves that
  library to another number to make room.
* **The second sound library** is the successor of the first and has the
  same number, so the two cannot be loaded together. The
  variant **rwe-1999-ahi** has the second one instead of the first.

## Using them

After the build, the libraries are in
`build/amiga/variants/rwe-1999/userlibs/rwe/`, and the two exceptions in
the `userlibs/rwe/` directories of their own variants. To use them:

1. Take the `acidlibs` and `deflibs` of the same variant, from
   `build/amiga/variants/rwe-1999/`, for your `blitz2` directory.
2. Copy the userlibs you need into `blitz2:userlibs/rwe/`.
3. Start blitz2, or choose RELOAD ALL LIBS.

Each command's short help text is in the library and shown by the
editor's HELP key.
