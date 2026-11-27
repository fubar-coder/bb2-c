---
title: ilbmifflib
description: The bugs of Blitz Basic 2 in the command library ilbmifflib that
  the BB2 C port fixes.
sidebar:
  order: 31
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/ilbmifflib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### IFF files

These are the commands that load and save IFF files, the Amiga's standard
format for pictures (ILBM), colours, sounds and animations.

* `ILBMInfo` of a file that could not be opened closed it all the same,
  which crashes older versions of AmigaOS; only an open file is closed
  ([#186](https://github.com/fubar-coder/bb2-c/issues/186)).
* `DecodeILBM` did not skip the pad byte behind a chunk of odd length,
  such as a short text (ANNO) that paint programs write into the file, so
  it read the next chunk one byte too early and stopped with "Can't Find
  IFF 'BODY'" or took wrong data; the pad byte is skipped
  ([#189](https://github.com/fubar-coder/bb2-c/issues/189)).
* `DecodeILBM` wrote all planes, rows and bytes of the picture into the
  bitmap, also into a smaller one: through plane addresses behind the
  bitmap's depth and past the end of its planes. A packed run across the
  end of a row went on in the next row. Only the part of the picture that
  fits the bitmap is stored, and every run ends with its row
  ([#190](https://github.com/fubar-coder/bb2-c/issues/190),
  [#191](https://github.com/fubar-coder/bb2-c/issues/191)).

::fixed-bugs-library{part="elsewhere"}
