---
title: Libraries
weight: 3
description: Fixed bugs of Blitz Basic 2 in the command libraries, in their error handling without the debugger, in AmigaOS calls and in the RWE userlibs.
---

These bugs are in the libraries that provide the commands of Blitz Basic
2 ([Libraries]({{< relref "/docs/libraries" >}})). A fix in a library
changes only that library, and a program behaves differently only where
it ran into the bug. The first section shows the one an ordinary program can run into;
the other fixes follow as a list, one sentence each with a link to its
issue.

## Dates

### `Date$` showed 1 January after a leap year as 32 December {#date-after-leap-year}

`Date$` turns a day number, the days since 1 January 1978 that
`SystemDate` returns, into a date string. For one day in every four years
it gave a date that does not exist: the 1 January after each leap year,
such as 1 January 1981 or 1 January 2025, came out as the 32nd of
December of the leap year. A program that shows today's date showed it
wrong on that day.

**What happened**

```
NPrint Date$(1095)
NPrint Date$(1096)
NPrint Date$(1097)
End
```

Day 1096 is 1 January 1981. The original prints

```
31/12/1980
32/12/1980
02/01/1981
```

`Days`, `Months` and `Years` after that `Date$` returned 32, 12 and 1980
as well.

**With the port**

```
31/12/1980
01/01/1981
02/01/1981
```

and `Days`, `Months` and `Years` return 1, 1 and 1981.

Issue: [Date$ showed the 1st of January after a leap year as "32/12" (#120)](https://github.com/fubar-coder/bb2-c/issues/120).

## Other fixes in the command libraries

### Lists and text

* `Dim List` with size 0 wrote behind its memory block; nothing is
  written there ([#59](https://github.com/fubar-coder/bb2-c/issues/59)).
* Characters far above the font's first character, among them the umlauts
  of the standard font, were not drawn; they are drawn
  ([#148](https://github.com/fubar-coder/bb2-c/issues/148)).

### GadTools gadgets

* A gadget created without an optional value, such as its start value,
  took over the value of the gadget created before it; it starts with the
  default ([#169](https://github.com/fubar-coder/bb2-c/issues/169)).
* All `GTShape` gadgets showed the images of the last one, and GadTools'
  own image was overwritten; each gadget keeps its own images
  ([#173](https://github.com/fubar-coder/bb2-c/issues/173)).
* `GTShape` wrote to low memory when the gadget could not be created; a
  failed gadget is left alone
  ([#327](https://github.com/fubar-coder/bb2-c/issues/327)).
* `GTPalette` with a colour ignored the colour and broke a following tag
  list; the colour is set
  ([#168](https://github.com/fubar-coder/bb2-c/issues/168)).
* Cycle and MX gadgets had room for a limited number of labels without a
  check, and their memory was never freed; the labels are counted and the
  memory is freed with the gadget
  ([#174](https://github.com/fubar-coder/bb2-c/issues/174)).

### Windows

* Closing one window discarded the events of all other windows; only the
  closed window's events are discarded
  ([#322](https://github.com/fubar-coder/bb2-c/issues/322)).
* `WMove` and `WSize` hung when Intuition limited the new position or
  size; they wait only briefly
  ([#320](https://github.com/fubar-coder/bb2-c/issues/320)).
* Window titles were copied and never freed, so every window and every new
  title cost memory until the program ended; the copies are freed with the
  window or the next title
  ([#318](https://github.com/fubar-coder/bb2-c/issues/318)).
* `WindowTags` with a long tag list overwrote code of the library; a long
  list is copied only in part and the rest is linked
  ([#317](https://github.com/fubar-coder/bb2-c/issues/317)).

### Screens and palettes

* Faded and decoded palettes were wrong or black on Kickstart before 3.0;
  the colours are right
  ([#224](https://github.com/fubar-coder/bb2-c/issues/224)).
* A screen mode with a monitor part, such as PAL or NTSC, opened on the
  default monitor; the whole mode is used
  ([#250](https://github.com/fubar-coder/bb2-c/issues/250)).

## Runtime errors without the debugger

Every library has an area of error checks that is left out when a
program is compiled without runtime error checking. Several libraries had
error handling that jumped into that area, or kept the text of an error
message there. Without runtime error checking, such an error jumped to
memory behind the library, or reported a message that pointed into
nowhere. The port puts this code and these messages where they belong, so
the error is reported with its proper text in both cases. With runtime
error checking on, nothing changes.

The fixes concern:

* [memory errors, "Out of Memory!" (#956)](https://github.com/fubar-coder/bb2-c/issues/956)
* [fonts, "Can't Open Font" and others (#955)](https://github.com/fubar-coder/bb2-c/issues/955)
* [screens, "Can't open Screen" and others (#251)](https://github.com/fubar-coder/bb2-c/issues/251)
* [displays and copper lists (#412)](https://github.com/fubar-coder/bb2-c/issues/412)
* [shapes, `DecodeShapes` of address 0 (#270)](https://github.com/fubar-coder/bb2-c/issues/270) and [the shape error messages (#953)](https://github.com/fubar-coder/bb2-c/issues/953)
* [sprite files (#954)](https://github.com/fubar-coder/bb2-c/issues/954)
* [`SetPeriod` without a sound, and the sound error messages (#70)](https://github.com/fubar-coder/bb2-c/issues/70)
* [animations, "Can't Open Anim File" (#398)](https://github.com/fubar-coder/bb2-c/issues/398)
* [music modules, "Error Accessing Module File" (#957)](https://github.com/fubar-coder/bb2-c/issues/957)
* [IFF pictures (#192)](https://github.com/fubar-coder/bb2-c/issues/192) and [saving IFF files, "Can't open File" (#185)](https://github.com/fubar-coder/bb2-c/issues/185)
* [Workbench arguments without memory at the start of a program (#305)](https://github.com/fubar-coder/bb2-c/issues/305)

## AmigaOS calls

These fixes concern the calls of AmigaOS functions
([AmigaOS libraries]({{< relref "/docs/libraries/amigaos" >}})).

* A program hung at its start if an AmigaOS library it calls was missing;
  it starts, and only a call of the missing library then crashes
  ([#44](https://github.com/fubar-coder/bb2-c/issues/44)).
* A program that calls a resource, such as the battery clock or the CIA
  resources, hung at its start; the resource is opened
  ([#45](https://github.com/fubar-coder/bb2-c/issues/45)).
* `CachePreDMA_` and `CachePostDMA_` passed their flags in the wrong
  register; the flags reach the system
  ([#959](https://github.com/fubar-coder/bb2-c/issues/959)).

## RWE userlibs

These fixes concern the
[RWE userlibs]({{< relref "/docs/libraries/userlibs" >}}).

* `RIAnimInit`, `RIAnimDepth`, `RIAnimWidth` and `RIAnimHeight` crashed on
  some data that is not a `FORM ANIM`, such as an IFF `LIST` of type
  `ANIM`; they return normally, and for such a `LIST` `RIAnimDepth` returns
  0 ([#513](https://github.com/fubar-coder/bb2-c/issues/513)).
