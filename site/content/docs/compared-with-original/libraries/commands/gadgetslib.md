---
title: gadgetslib
weight: 26
description: The bugs of Blitz Basic 2 in the command library gadgetslib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/gadgetslib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Gadgets

* `SelectMode` had no effect, so `SelectMode 1` did not give new gadgets
  a box as their highlight; it does
  ([#952](https://github.com/fubar-coder/bb2-c/issues/952)).
* `Free GadgetList` did not free the text buffers of string gadgets and
  the data of prop gadgets, which stayed allocated until the program
  ended; they are freed with the list
  ([#163](https://github.com/fubar-coder/bb2-c/issues/163)).
* `StringGadget` after `Borders Off` wrote to the addresses 0, 2, 36 and
  38; it writes nothing there
  ([#164](https://github.com/fubar-coder/bb2-c/issues/164)).
* `StringGadget` with a `Maxlen` of 0 cleared 65536 bytes of memory and
  crashed the program; with runtime error checking it is the error
  "Maxlen MUST be greater than zero"
  ([#165](https://github.com/fubar-coder/bb2-c/issues/165)).
* A `ShapeGadget` of a shape of depth 0 got a broken image; with runtime
  error checking such a shape cannot be made, as `InitShape` refuses a
  depth of 0
  ([#167](https://github.com/fubar-coder/bb2-c/issues/167)).

{{% fixed-bugs-library part="elsewhere" %}}
