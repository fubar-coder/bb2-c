---
title: menuslib
description: The bugs of Blitz Basic 2 in the command library menuslib that the
  BB2 C port fixes.
sidebar:
  order: 42
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/menuslib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Menus

* A menu, item or subitem with an empty text read its text from address
  0, and the item got the width of whatever was there; the text is empty,
  and an empty item is as wide as its gaps
  ([#981](https://github.com/fubar-coder/bb2-c/issues/981)).
* `MenuState` of a single menu changed the wrong bit, so a menu could not
  be disabled or enabled on its own; it can
  ([#216](https://github.com/fubar-coder/bb2-c/issues/216)).
* `MenuChecked` of any entry but the first item and subitem of a menu
  walked the menu lists again and gave the state of another entry, or
  read unrelated memory; it gives the state of the entry asked for
  ([#217](https://github.com/fubar-coder/bb2-c/issues/217)).
* `ShapeItem` and `ShapeSub` ignored small flag values such as toggle,
  exclude and checked; they take their flags as `MenuItem` does
  ([#218](https://github.com/fubar-coder/bb2-c/issues/218)).
* Item flags such as $10000 were taken as 4, a checked toggle item that
  excludes all others; they are taken as given
  ([#219](https://github.com/fubar-coder/bb2-c/issues/219)).
* Replacing a subitem moved the subitems behind it by a wrong amount, so
  they covered it or left a gap; they move by the change of its height
  ([#220](https://github.com/fubar-coder/bb2-c/issues/220)).

::fixed-bugs-library{part="elsewhere"}
