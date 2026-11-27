---
title: popupwinlib
description: The bugs of Blitz Basic 2 in the RWE userlib popupwinlib that the
  BB2 C port fixes.
sidebar:
  order: 4
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/popupwinlib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in popupwinlib {#popupwinlib-fixes}

These fixes concern
[popupwinlib](/docs/libraries/userlibs/popupwinlib/),
the popup menus of Reflective Images.

* Programs that use the library run on Kickstart 1.x, and the menus work
  there. The library builds its message port and the memory of a menu
  level with functions that every Kickstart has, and below Kickstart 2.0
  it opens the menu windows in the way Kickstart 1.x knows. On
  Kickstart 1.x a sub menu whose window does not fit on the screen, at
  its right or bottom edge, does not open, and the whole menu closes.
  The original called functions of Kickstart 2.0, and every program with
  the library crashed at its start
  ([#1140](https://github.com/fubar-coder/bb2-c/issues/1140)).
* `PopUpWindow` can be used as a function, `r=PopUpWindow(Screen#,*items)`,
  and returns the address of the item chosen, or 0 if no item was
  highlighted when the mouse button was pressed; the original was a
  statement only and never told the program which item was chosen. As a
  statement it is the same as before
  ([#471](https://github.com/fubar-coder/bb2-c/issues/471)).
* A mouse button pressed while a sub menu of `PopUpWindow` closes is no
  longer lost; the original threw away the waiting messages of all menu
  levels when it closed one, and the menu stayed open until the next
  click ([#469](https://github.com/fubar-coder/bb2-c/issues/469)).
* When there is no memory for a menu level, `PopUpWindow` closes the menu
  and returns; the original went on with the level's data at address 0,
  wrote over the start of memory and could crash
  ([#467](https://github.com/fubar-coder/bb2-c/issues/467)).
* `PopUpInit` keeps the message port the library already has; the
  original made a new one at each call and lost the old one with its
  signal, and once the program's signals were used up, the next
  `PopUpWindow` hung with the menu open
  ([#470](https://github.com/fubar-coder/bb2-c/issues/470)).
* `PopUpWindow` leaves the audio filter and the power LED as they are;
  the original switched both over for every menu with sub menus it
  showed
  ([#468](https://github.com/fubar-coder/bb2-c/issues/468)).

::fixed-bugs-library{part="elsewhere"}
