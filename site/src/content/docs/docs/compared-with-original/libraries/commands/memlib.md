---
title: memlib
description: The bugs of Blitz Basic 2 in the command library memlib that the
  BB2 C port fixes.
sidebar:
  order: 41
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/memlib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Arrays and sorting

* A program that caught "Out of Memory!", for example of a `Dim`, with
  `SetErr` and went on lost the list of its memory blocks, so its next
  allocation overwrote the pointer to exec at address 4, which the whole
  system uses; the error frees the memory but keeps the list, so later
  allocations work
  ([#1071](https://github.com/fubar-coder/bb2-c/issues/1071)).

::fixed-bugs-library{part="elsewhere"}
