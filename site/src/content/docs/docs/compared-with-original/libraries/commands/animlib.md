---
title: animlib
description: The bugs of Blitz Basic 2 in the command library animlib that the
  BB2 C port fixes.
sidebar:
  order: 2
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/animlib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Animations

These are the anim objects, which play IFF ANIM files on bitmaps.

* `NextFrame` of an anim with a single change read its next change from
  address 0, and so did the first `NextFrame` of an anim without any; such
  an anim plays once and stays on its last frame
  ([#401](https://github.com/fubar-coder/bb2-c/issues/401)).
* `LoadAnim` of a file without a first picture (BODY) took the value for
  "not found" as the length of the picture, and `InitAnim` then unpacked
  whatever lay in memory; such an anim has no first picture, and
  `InitAnim` draws nothing
  ([#400](https://github.com/fubar-coder/bb2-c/issues/400)).
* An anim whose file gives a width, height or depth of 0 made `InitAnim`
  and `NextFrame` run their loops 65536 times and write over memory; such
  an anim is loaded without picture data, and they draw nothing
  ([#402](https://github.com/fubar-coder/bb2-c/issues/402)).
* `InitAnim` unpacked every first picture as packed data, also one the
  file stores unpacked, and a packed run longer than the rest of its row
  was written behind the row; an unpacked first picture is copied, and
  every run ends with its row
  ([#397](https://github.com/fubar-coder/bb2-c/issues/397)).
* Freeing an anim read the link to its next change from memory it had
  just freed; the link is read first
  ([#399](https://github.com/fubar-coder/bb2-c/issues/399)).

::fixed-bugs-library{part="elsewhere"}
