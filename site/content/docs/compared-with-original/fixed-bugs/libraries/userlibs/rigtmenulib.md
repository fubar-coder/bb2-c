---
title: rigtmenulib
weight: 26
description: The bugs of Blitz Basic 2 in the RWE userlib rigtmenulib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rigtmenulib {#rigtmenulib-fixes}

These fixes concern
[rigtmenulib]({{< relref "/docs/libraries/userlibs/rigtmenulib" >}}),
the GadTools menus by Reflective Images. A program builds its menus in
a list with `GTMenuTitle`, `GTMenuItem` and `GTSubItem`, removes entries
with `GTFreeMenu`, and `CreateMenuStrip` makes a menu strip of the list.

* `GTMenuItem` in the form with `UserData` gives the item the
  `mutualexclude` mask of the call; the original gave it the mask of the
  item or sub item added before
  ([#692](https://github.com/fubar-coder/bb2-c/issues/692)).
* `GTFreeMenu` with an item or sub item number removes that item or sub
  item. The original decided whether the number was given by the upper
  half of a register that the compiled program had left as it happened
  to be, so it could remove the whole item or the whole title instead
  ([#691](https://github.com/fubar-coder/bb2-c/issues/691)).
* `GTFreeMenu` of a title, and freeing a `GTMenuList` object, free the
  title's text too; the original kept the text until the end of the
  program, so a program that built its menus again and again lost
  memory ([#695](https://github.com/fubar-coder/bb2-c/issues/695)).
* A program that uses the library closes gadtools.library at its end;
  the original left it open, once more with every run
  ([#690](https://github.com/fubar-coder/bb2-c/issues/690)).
* `CreateMenuStrip` on a list without titles frees the object's old
  strip and returns 0, so a later `GTSetMenu` attaches no menus. It also
  returns 0, with no strip, when GadTools cannot make or lay out the
  strip. The original read a title from address 0, made a strip of what
  it found there and returned -1, also when GadTools had failed
  ([#689](https://github.com/fubar-coder/bb2-c/issues/689)).
* `GTMenuItem` and `GTSubItem` keep a `shortcut$` whose text lies at an
  address divisible by 65536; the original took it for no shortcut, and
  the item had no key
  ([#694](https://github.com/fubar-coder/bb2-c/issues/694)).
* With runtime error checking, `GTFreeMenu` with a negative item number
  stops with "Illegal Menu Item number"; the original freed the title as
  if it were an item and damaged memory. A negative sub item number
  removes the whole item, as in the original
  ([#1089](https://github.com/fubar-coder/bb2-c/issues/1089)).

{{% fixed-bugs-library part="elsewhere" %}}
