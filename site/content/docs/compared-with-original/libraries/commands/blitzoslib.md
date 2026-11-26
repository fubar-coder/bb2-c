---
title: blitzoslib
weight: 12
description: The bugs of Blitz Basic 2 in the command library blitzoslib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/blitzoslib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Blitz mode boxes

The Blitz OS commands give a Blitz mode program pages of boxes as its
user interface
([Blitz OS]({{< relref "/docs/libraries/commands/blitzoslib" >}})).

* `AddBox` of a string box 8 to 15 pixels wide, whose text has room for
  one byte, wrote 65536 spaces behind that byte; it writes none
  ([#84](https://github.com/fubar-coder/bb2-c/issues/84)).
* On a page of more than 1024 boxes, the box numbers from 1024 on reached
  the memory before the box table or, from 2048 on, the boxes from 0 on,
  and `BoxSelected` returned a wrong number; every box number reaches its
  own box, and `BoxSelected` returns it
  ([#81](https://github.com/fubar-coder/bb2-c/issues/81)).
* `BoxString` of an answer of spaces only, such as that of a string box
  never typed into, removed the spaces beyond the start of the answer
  and could copy up to 65535 bytes into the string buffer; it returns the
  empty string
  ([#85](https://github.com/fubar-coder/bb2-c/issues/85)).
* A box added again with `AddBox` without a text printed the memory from
  address 0 on when it was drawn; it prints nothing
  ([#89](https://github.com/fubar-coder/bb2-c/issues/89)).
* Cursor up or down in `BlitzEvent` with no box selected, such as right
  after `InitPage`, read low memory as a box and could write to it; it
  does nothing
  ([#90](https://github.com/fubar-coder/bb2-c/issues/90)).
* In a string box, Backspace at the start of the text and control keys
  such as Return, Esc and Tab turned the cursor off and left it off; it
  is drawn again
  ([#91](https://github.com/fubar-coder/bb2-c/issues/91)).
* `DeSelectBox` left the box selected, so it was still drawn as selected
  and `BoxSelected` still returned it; the selection is cleared, and
  `BoxSelected` returns -1
  ([#94](https://github.com/fubar-coder/bb2-c/issues/94)).
* `BoxPrint Box#,Bank#,Line` replaced the line feed at the end of the
  printed line by a 0 in the bank itself; the bank stays as it is
  ([#87](https://github.com/fubar-coder/bb2-c/issues/87)).
* A list box made with `AddBox` and a text, or given a text with
  `SetBoxTitle`, got no entries: it showed nothing, and `SetBox` on it
  always failed with "Value Out of Range"; it gets one entry for each part
  of the text
  ([#989](https://github.com/fubar-coder/bb2-c/issues/989)).
* `SpotBox` on a page that was not made with `InitPage`, was freed or has
  no boxes searched 65536 boxes in the memory behind the page and could
  return one found there; with runtime error checking it is the error
  "UnInitialised Page"
  ([#83](https://github.com/fubar-coder/bb2-c/issues/83)).
* `SelectBox Box#` with a number the page does not have selected and drew
  whatever lay behind the box table; with runtime error checking it is
  the error "Illegal Box# For Page"
  ([#82](https://github.com/fubar-coder/bb2-c/issues/82)).
* `InitPage` with 32768 boxes or more took the number as a negative one
  and asked for a box table of a wrong size, which stopped the program
  with "Out of Memory!"; with runtime error checking it is the error
  "Illegal Number of Boxes"
  ([#1043](https://github.com/fubar-coder/bb2-c/issues/1043);
  [`InitPage` with 32768 boxes or more]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#initpage-numboxes" >}})).

{{% fixed-bugs-library part="elsewhere" %}}
