---
title: screenslib
weight: 49
description: The bugs of Blitz Basic 2 in the command library screenslib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/screenslib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Windows and screens

* A screen mode with a monitor part, such as PAL or NTSC, opened on the
  default monitor; the whole mode is used
  ([#250](https://github.com/fubar-coder/bb2-c/issues/250)).
* `Screen`, `ScreenTags`, `WbToScreen` and `FindScreen` on a screen
  object that had used `ShowBitMap`, on Kickstart 3.0 or later, crashed
  the program when they freed the old screen; it is freed properly
  ([#247](https://github.com/fubar-coder/bb2-c/issues/247)).
* `FindScreen`, `WbToScreen` and `CloseScreen` on an object that had only
  found a screen another object had opened closed that screen, and the
  other object was left with a closed screen; a found screen is only
  forgotten
  ([#248](https://github.com/fubar-coder/bb2-c/issues/248)).
* `ScreenTags` with the address of a tag list of more than 15 tags
  overwrote code behind the library; up to 15 tags are copied and the
  rest of the list is linked, as for `WindowTags`
  ([#249](https://github.com/fubar-coder/bb2-c/issues/249)).
* `ScreenTags` with 16 or more tag and data pairs as arguments wrote
  behind its tag buffer, over code of the libraries; with runtime error
  checking more than 15 pairs are the error "Too Many Tags", as for
  `WindowTags`
  ([#1039](https://github.com/fubar-coder/bb2-c/issues/1039)). A longer
  list can be given as the address of a tag list.

{{% fixed-bugs-library part="elsewhere" %}}
