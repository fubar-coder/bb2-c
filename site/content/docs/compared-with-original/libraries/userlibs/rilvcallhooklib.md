---
title: rilvcallhooklib
weight: 30
description: The bugs of Blitz Basic 2 in the RWE userlib rilvcallhooklib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/rilvcallhooklib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rilvcallhooklib {#rilvcallhooklib-fixes}

These fixes concern
[rilvcallhooklib]({{< relref "/docs/libraries/userlibs/rilvcallhooklib" >}}),
a render hook by Red When Excited that draws the items of a GadTools
listview as columns of texts and bitmaps, with an optional indent drawn
as dotted tree lines.

* The tree lines of an indented item depend on its indent alone. The
  original computed their number with the upper half of a register that
  the caller of the hook had left, so the lines could be missing, or
  65536 lines were drawn for an indent below 16 pixels
  ([#737](https://github.com/fubar-coder/bb2-c/issues/737)).
* The last tree line of an item is short when the next item ends the
  list or has a smaller indent. The original read an unrelated word of
  the next node instead of its indent, so the line was short or long by
  chance, and with an odd width of the first field it crashed a 68000
  ([#739](https://github.com/fubar-coder/bb2-c/issues/739)).
* A right-aligned bitmap is drawn at the right edge of its field. The
  original drew it too far to the right, by the field's horizontal
  position, and filled the field with the background pen instead
  ([#740](https://github.com/fubar-coder/bb2-c/issues/740)).
* A text field whose pointer is 0 stays empty; the original showed the
  bytes at address 0 as text
  ([#742](https://github.com/fubar-coder/bb2-c/issues/742)).
* After drawing an indented item, the hook puts the line pattern of the
  RastPort back as it found it. The original set it to solid lines, or
  left it dotted after an indent of 1 to 15 pixels, which has no tree
  lines; later lines that the program drew on the same RastPort changed
  with it. In a GadTools listview the hook gets a RastPort of its own,
  and the tree lines look as before
  ([#738](https://github.com/fubar-coder/bb2-c/issues/738)).

{{% fixed-bugs-library part="elsewhere" %}}
