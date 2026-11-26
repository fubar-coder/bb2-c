---
title: ridoslib
weight: 19
description: The bugs of Blitz Basic 2 in the RWE userlib ridoslib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/ridoslib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ridoslib {#ridoslib-fixes}

These fixes concern
[ridoslib]({{< relref "/docs/libraries/userlibs/ridoslib" >}}), the DOS
helpers of Red When Excited: file notification, the list of devices,
volumes and assigns, and splitting a path; the fix of
[the text that follows `GetPathName` and `NextDevice`]({{< relref "/docs/compared-with-original/libraries#text-after-string-function" >}})
is described for several libraries together.

* Programs that use the library run on Kickstart 1.x. There
  `GetFileName` and `GetPathName` work, and the commands of file
  notification and of the device list, which need Kickstart 2.0, do
  nothing: `AddNotify`, `NotifyPort`, `CheckNotify`, `WaitNotify` and
  `GetDeviceList` return 0, `NextDevice` returns "" and
  `DeviceEntryType` -1. The original called functions of Kickstart 2.0
  at the start of the program, and every program with the library
  crashed at its start
  ([#1142](https://github.com/fubar-coder/bb2-c/issues/1142)).

File notification:

* `AddNotify` with a slot that is in use ends the notification in it
  first, as `RemNotify` does; the original lost it, so its messages kept
  coming and it could no longer be ended
  ([#620](https://github.com/fubar-coder/bb2-c/issues/620)).
* `AddNotify` returns 0 when there is not enough memory; the original
  wrote the file name or the request to address 0
  ([#619](https://github.com/fubar-coder/bb2-c/issues/619)).
* A notification that DOS cannot start is freed at once, and the end of
  the program frees every notification; the original kept such a request
  in its slot, and at the end of the program could stop freeing before
  it ([#621](https://github.com/fubar-coder/bb2-c/issues/621)).

The device list:

* The end of the program unlocks the device list if `FreeDeviceList` was
  not called, and a second `GetDeviceList` unlocks the list before it
  locks it again. The original left the list locked in both cases, and
  every program that wanted to change it, such as `Assign`, a disk
  change or mounting a device, waited for ever
  ([#1081](https://github.com/fubar-coder/bb2-c/issues/1081)).
* `FreeDeviceList` without a locked list does nothing; the original
  unlocked it all the same, which made the system raise an alert and
  left the program's later `Forbid` calls without effect
  ([#623](https://github.com/fubar-coder/bb2-c/issues/623)).
* `NextDevice` without a locked list returns ""; the original returned a
  string of an unpredictable length
  ([#625](https://github.com/fubar-coder/bb2-c/issues/625)).
* `DeviceEntryType` after `FreeDeviceList` returns the kind of the entry
  `NextDevice` returned last; the original read that entry from the
  unlocked list, where it could already have been removed
  ([#622](https://github.com/fubar-coder/bb2-c/issues/622)).

Splitting a path:

* `GetPathName` looks at the first character too, so `GetPathName("/abc")`
  and `GetPathName(":x")` return "/" and ":"; the original returned ""
  ([#624](https://github.com/fubar-coder/bb2-c/issues/624)).
* `GetFileName` and `GetPathName` take parts of any length; the original
  overwrote the code of the library with a part of 256 characters or
  more ([#627](https://github.com/fubar-coder/bb2-c/issues/627)).
* `GetFileName` and `GetPathName` work with paths of 65536 characters or
  more; the original read only the low 16 bits of their length, so it
  returned wrong characters and `GetPathName` looked at only part of
  the path ([#628](https://github.com/fubar-coder/bb2-c/issues/628)).

#### A notification slot outside 0 to 63 {#notify-slot-range}

**What happened.** `AddNotify` and `RemNotify` of
[ridoslib]({{< relref "/docs/libraries/userlibs/ridoslib" >}}) keep each
notification in a slot of a table of 64 entries, numbered 0 to 63. The
original did not check the slot number, also not with runtime error
checking: with a number outside, `AddNotify` wrote its request over the
library's data around the table, and `RemNotify` took that data for a
request, ended it and freed it.

**With the port.** With runtime error checking, a slot outside 0 to 63
is the error "Notify number must be 0 to 63", for example in
`AddNotify(-1,"Work:file")`. Without runtime error checking, the
commands are as before.

**What to change.** Use the slots 1 to 63. Slot 0 is allowed, but
`CheckNotify` also returns 0 when no message is waiting.

Issue: [AddNotify/RemNotify do not check the notification id (64 table entries) (#626)](https://github.com/fubar-coder/bb2-c/issues/626).

{{% fixed-bugs-library part="elsewhere" %}}
