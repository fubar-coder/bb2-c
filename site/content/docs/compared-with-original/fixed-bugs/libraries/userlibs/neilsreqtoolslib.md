---
title: neilsreqtoolslib
weight: 3
description: The bugs of Blitz Basic 2 in the RWE userlib neilsreqtoolslib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `RTEZGetLong` offered the number of the call before {#rtezgetlong-previous}

[neilsreqtoolslib]({{< relref "/docs/libraries/userlibs/neilsreqtoolslib" >}})
makes the requesters of reqtools.library usable from Blitz Basic 2.
`RTEZGetLong(Title$,BodyText$ [,DefaultValue])` asks the user for a
number; `DefaultValue` is the number the input field starts with.

**What happened.** The library kept the number of one call for the
next. A call without `DefaultValue` started with the number of the call
before, entered or given as its default, and when the user cancelled it
returned that number as if it had been entered. This program asks
twice; the user enters 42 in the first requester and cancels the
second:

```
n=RTEZGetLong("Age","How old are you?",30)
NPrint n
m=RTEZGetLong("Count","How many?")
NPrint m
```

The outputs are worked out from the library's code, not recorded from a
run of this program; the library's test checks the same with a stand-in
for reqtools.library.

| | With the original | With the port |
|---|---|---|
| The second requester starts with | 42 | 0 |
| The program prints | `42` and `42` | `42` and `0` |

**With the port.** A call without `DefaultValue` starts with 0 and
returns 0 when the user cancels. A call with `DefaultValue` returns the
default when the user cancels, as before. `RTReturnValue` is 0 after a
cancel and tells it from a number that was entered.
`RTEZGetLongRange` also starts with 0 without `DefaultValue`, even when
0 lies outside `Min` and `Max`; reqtools then asks for a number in the
range when the user confirms. It returned 0 on a cancel already.

**What to change.** A program that wants the requester to offer the
number entered last gives it as `DefaultValue`:
`m=RTEZGetLong("Count","How many?",m)`.

Issues:

* [RTEZGetLong returns the value also when the requester is cancelled (#457)](https://github.com/fubar-coder/bb2-c/issues/457)
* [RTEZGetLongRange without a default shows the previous call's number (#1078)](https://github.com/fubar-coder/bb2-c/issues/1078)

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in neilsreqtoolslib {#neilsreqtoolslib-fixes}

These fixes concern
[neilsreqtoolslib]({{< relref "/docs/libraries/userlibs/neilsreqtoolslib" >}}),
the requesters of reqtools.library; the fix of `RTEZGetLong` and
`RTEZGetLongRange` is [described above](#rtezgetlong-previous).

* Without reqtools.library, or when the library could not prepare its
  requesters at the start, the commands return what they return when
  the user cancels: 0, an empty string, -1 for `RTEZPaletteRequest`, and
  0 for `RTVersion` and `RTRevision`; the original crashed the machine at
  the first requester. `IsReqToolsActive` still tells whether the
  requesters are there
  ([#463](https://github.com/fubar-coder/bb2-c/issues/463)).
* When the library cannot prepare its requesters at the start for lack
  of memory, it closes reqtools.library and frees the requesters it has;
  the original kept both after the program ended
  ([#460](https://github.com/fubar-coder/bb2-c/issues/460)).
* `RTEZMultiLoadFile` returns -1 whenever files were chosen; the original
  returned the address of the list of files instead when that address
  happened to end in the byte $00
  ([#462](https://github.com/fubar-coder/bb2-c/issues/462)).
* `RTReturnValue` gives the result of `RTEZPaletteRequest` and
  `RTASyncPaletteRequest` too; after these the original still gave that
  of the requester before
  ([#464](https://github.com/fubar-coder/bb2-c/issues/464)).
* `RTEZFlagsRequest` shows the title `Title$`, and an empty title gives
  reqtools' default title; the original always showed the default title
  ([#455](https://github.com/fubar-coder/bb2-c/issues/455)).
* `RTEZFreePattern 3` removes the pattern gadget that `RTEZSetPattern 3`
  added to the multi-load requester, and `RTEZFreePattern 2` does
  nothing, as `RTEZSetPattern 2`; the original removed the gadget for 2
  and did nothing for 3. A program that used 2 for the multi-load
  requester has to give 3
  ([#456](https://github.com/fubar-coder/bb2-c/issues/456)).
* A cancelled `RTEZGetString` frees the memory of its input; the original
  kept `MaxChars` bytes for every cancelled call until the program ended
  ([#458](https://github.com/fubar-coder/bb2-c/issues/458)).
* `RTEZScreenModeRequest` opens the screen mode requester through the
  function of reqtools.library that is meant for it. The original called
  the function of the font requester instead; reqtools.library 38 opens
  the screen mode requester for that call all the same, so with it
  nothing visible changes
  ([#466](https://github.com/fubar-coder/bb2-c/issues/466)).
* `RTASyncRequest` and `RTASyncPaletteRequest` no longer read a value
  through the window object that they never used, and
  `RTCheckASyncRequest`, `RTEndASyncRequest` and the palette commands of
  the same names give reqtools the signals of the requester. The
  requesters worked with the original as well: reqtools.library 38 does
  not use these signals, so with it nothing visible changes
  ([#454](https://github.com/fubar-coder/bb2-c/issues/454)).

{{% fixed-bugs-library part="elsewhere" %}}
