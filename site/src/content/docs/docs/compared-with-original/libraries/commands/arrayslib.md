---
title: arrayslib
description: The bugs of Blitz Basic 2 in the command library arrayslib that the
  BB2 C port fixes.
sidebar:
  order: 4
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/arrayslib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Arrays and sorting

#### A `Dim` of 4 GB or more {#dim-too-large}

**What happened.** Runtime error checking checks each dimension of a
`Dim`, but not the size of the whole array. An array of 4 GB or more,
such as `Dim a.l(32767,32767)`, got a wrong, tiny size of a few bytes,
and the `Dim` succeeded.

**With the port.** Such a `Dim` fails with "Out of Memory!". With
runtime error checking, it is the error "Array Size out of Range", at
the dimension that makes the array too large.

**What to change.** Make the array smaller; no Amiga has the memory for
it.

Issue: [A Dim of 4 GB or more got an array of a few bytes (#58)](https://github.com/fubar-coder/bb2-c/issues/58).

#### A `Dim` with the highest index 65535 {#dim-65535}

**What happened.** A dimension of an array or list has at most 65535
elements, the indexes 0 to 65534. `Dim a(65535)` and `Dim List a(65535)`
gave an array or list of no elements, so every access to it lay outside
its memory. Runtime error checking refused only an index of 65536 and
more.

**With the port.** With runtime error checking, the highest index 65535
is the error "Array Dimension out of Range" too.

**What to change.** Make the highest index 65534 or less, or spread the
elements over two dimensions.

The manual states no limit, but the original has one, and the port keeps
it: `Dim` counts a dimension's elements in a word. Without runtime error
checking a larger `Dim` is not refused but silently too small
(`Dim a.l(70000)` gets 17860 bytes, 4465 elements, in the original and in
the port), and an index of 65536 or more reaches outside the array or
wraps into it. Such a size or index is a programming error; turn runtime
error checking on to find it ("Array Dimension out of Range", "Array
Subscript out of Range").

Issue: [Dim a(65535) gave an array of no elements (#1037)](https://github.com/fubar-coder/bb2-c/issues/1037).

### Lists

* `Dim List` with size 0 wrote behind its memory block; nothing is
  written there ([#59](https://github.com/fubar-coder/bb2-c/issues/59)).

::fixed-bugs-library{part="elsewhere"}
