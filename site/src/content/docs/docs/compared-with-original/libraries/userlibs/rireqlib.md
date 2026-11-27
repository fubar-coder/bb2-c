---
title: rireqlib
description: The bugs of Blitz Basic 2 in the RWE userlib rireqlib that the BB2
  C port fixes.
sidebar:
  order: 37
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/rireqlib/
---

::fixed-bugs-library

## Changes you may notice

### The file requester of `ReqFileRequest` opened with other flags {#reqfilerequest-default-flags}

`ReqFileRequest` of
[rireqlib](/docs/libraries/userlibs/rireqlib/) opens the
file requester of req.library, a requester library that is not part of
AmigaOS. Flags decide what the requester offers, such as the selection
of several files or a gadget that hides `.info` files. `ReqFlags` sets
them; until a program does, the library uses its own default flags.

**What happened.** The library is written to start with three flags:
directory caching, the gadget that hides `.info` files, and the purge of
the cache when a directory changes. The original combined the numbers
of these flag bits instead of their values and so started with three
other flags: show `.info` files, extended select of several files, and
directory caching.

**With the port.** A program that does not call `ReqFlags` gets the
requester the library means: with the gadget for `.info` files, a cache
that is purged when a directory changes, and without extended select.

**What to change.** A program that relies on the requester of the
original sets its flags itself, with `ReqFlags %111` or the flags
argument of `ReqFileRequest`.

Issue: [The default requester flags OR bit numbers (2|4|7 = 7) instead of bit masks (#787)](https://github.com/fubar-coder/bb2-c/issues/787).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rireqlib {#rireqlib-fixes}

These fixes concern
[rireqlib](/docs/libraries/userlibs/rireqlib/), the file
and colour requesters of req.library, a requester library that is not
part of AmigaOS, by Steven Matty; the fix of
[its default flags](#reqfilerequest-default-flags) is described above,
and the fix of [the text that follows `ReqFileRequest`](/docs/compared-with-original/libraries/#text-after-string-function)
for several libraries together.

* `ReqFileRequest` returns the path of the file the user chose. The
  original never gave the requester the library's buffers for the
  directory, the file and the path, so it returned only what the program
  itself had written to the path buffer at `ReqFileLoc`, or an empty
  string ([#785](https://github.com/fubar-coder/bb2-c/issues/785)).
* The library reserves req.library's whole file requester structure. The
  original's was too short, so the fields that the requester keeps there
  from call to call were written over the library's data behind it,
  among them the addresses of its buffers and the colour requester
  ([#1088](https://github.com/fubar-coder/bb2-c/issues/1088)).
* A title given to `ReqFileRequest` is for that call only; a later call
  without a title passes the library's empty title, as before the first
  call with one. In the original such a call showed whatever text had
  been put since in the memory where the earlier title had been. The
  flags of `ReqFileRequest(title$,flags)` stay set for later calls, as
  the guide documents
  ([#789](https://github.com/fubar-coder/bb2-c/issues/789)).
* Without req.library, `ReqFileRequest` returns an empty string and
  `ReqColour` 65535, the same as when the user cancels; the original
  crashed the system
  ([#788](https://github.com/fubar-coder/bb2-c/issues/788)).
* `ReqColour` opens the colour requester on the colour it is given,
  together with the fix of the next point; the original ignored it and
  started on colour 0
  ([#786](https://github.com/fubar-coder/bb2-c/issues/786)).
* `ReqColour` hands req.library the colour and the window of `ReqOutput`
  at the places where req.library reads them. The original's colour
  requester structure had another layout, so with req.library the
  requester started on the wrong colour and, after `ReqOutput`, did not
  open at all and returned 16387 at once
  ([#1091](https://github.com/fubar-coder/bb2-c/issues/1091)).
* A program run a second time from the editor gives the file requester
  no window until it calls `ReqOutput` again, the same as the colour
  requester. The original passed the window of the first run, which was
  closed when that run ended
  ([#1090](https://github.com/fubar-coder/bb2-c/issues/1090)).
* When a program ends, the library has req.library free the directory
  that the file requester keeps cached between calls. The original left
  that memory allocated after every run, as much as the last directory
  shown needed (with req.library 2.8, for example, 1560 bytes for
  `SYS:C`). About 40 KB that req.library itself allocates for its first
  file requester stay allocated until the system restarts; that is
  req.library's own behaviour, with any program that uses its file
  requester
  ([#1092](https://github.com/fubar-coder/bb2-c/issues/1092)).

::fixed-bugs-library{part="elsewhere"}
