---
title: ricommoditieslib
weight: 12
description: The bugs of Blitz Basic 2 in the RWE userlib ricommoditieslib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### A second `MakeCommodity` left a commodity behind {#makecommodity-second}

[ricommoditieslib]({{< relref "/docs/libraries/userlibs/ricommoditieslib" >}})
turns a program into a commodity: `MakeCommodity` registers it with the
system, which lists it in the Exchange program, and `SetHotKey` gives it
hotkeys. The messages of the commodity, from Exchange and from its
hotkeys, arrive at one message port of the program.

**What happened.** Every `MakeCommodity` made a new commodity with that
port, without removing the one made before. The first one stayed
registered and active, and at the end of the program only the last one
was removed. The first then kept the program's message port, which was
deleted, and the system crashed as soon as it sent the commodity a
message, for example that the list of commodities had changed. This
program calls `MakeCommodity` twice with two names:

```
If MakeCommodity("BugOrigTwiceA","BugOrig","first")
  NPrint MakeCommodity("BugOrigTwiceB","BugOrig","second")
Else
  NPrint "no commodity"
EndIf
End
```

**With the original**

```
-1
```

**With the port**

```
0
```

With the port, `MakeCommodity` returns 0 while the program has a
commodity, and the first one stays in use with its name, its texts and
its hotkeys.

**What to change.** A program that calls `MakeCommodity` once works as
before. One that called it again, for example to change the texts that
Exchange shows, gets 0 and keeps its first commodity; there is no
command to remove a commodity before the program ends, but `SetCxStatus
Off` deactivates it.

Issue: [MakeCommodity makes a new broker on every call; the previous one keeps the port that finit deletes (#560)](https://github.com/fubar-coder/bb2-c/issues/560).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ricommoditieslib {#ricommoditieslib-fixes}

These fixes concern
[ricommoditieslib]({{< relref "/docs/libraries/userlibs/ricommoditieslib" >}}),
the commodities by Reflective Images. `MakeCommodity` registers the
program as a commodity, `SetHotKey` adds hotkeys, `CommodityEvent` and
`WaitCommodity` fetch the next message, and `HotKeyHit`,
`ExchangeMessage` and the `Exchange` functions tell what it was. The fix
of [a second `MakeCommodity`](#makecommodity-second) is described above.

* The library answers the messages of the commodity: `CommodityEvent`
  and `WaitCommodity` answer the message fetched before, and the end of
  the program answers the last one and those still waiting. The
  original never answered a message, so commodities.library could not
  free it, and a running commodity lost about 100 bytes with every
  hotkey and every command of Exchange
  ([#561](https://github.com/fubar-coder/bb2-c/issues/561)).
* `WaitCommodity` takes the next message from the port, or waits for one
  if none is waiting. The original lost one waiting message and returned
  the next, which it left at the port, so the next `CommodityEvent`
  returned it a second time
  ([#566](https://github.com/fubar-coder/bb2-c/issues/566)).
* The `Exchange` functions return -1 only for a command of Exchange. The
  original compared the number of any message, so a press of hotkey 15
  counted as `ExchangeDisable`, the command 15
  ([#555](https://github.com/fubar-coder/bb2-c/issues/555)).
* `SetHotKey` frees the hotkey it replaces, and one that fails halfway
  frees what it had made. The original lost the memory of every hotkey
  it replaced, and a hotkey that failed halfway stayed in the commodity
  ([#563](https://github.com/fubar-coder/bb2-c/issues/563)).
* `SetHotKey` returns 0 for a hotkey description that
  commodities.library cannot understand. The original returned -1, and
  the hotkey never worked
  ([#565](https://github.com/fubar-coder/bb2-c/issues/565)).
* With runtime error checking, `SetHotKey` with a number outside 0 to
  15 stops with the runtime error "Hotkey number must be 0 to 15".
  Without it, such a number overwrites the library's other data, as in
  the original, and -1 removes the commodity
  ([#564](https://github.com/fubar-coder/bb2-c/issues/564)).
* `SetCxStatus` does nothing when `MakeCommodity` failed. The original
  then called commodities.library without a commodity and returned -1
  ([#562](https://github.com/fubar-coder/bb2-c/issues/562)).
* Without commodities.library, `MakeCommodity` returns 0. The original
  did not check that the library had been opened and crashed
  ([#559](https://github.com/fubar-coder/bb2-c/issues/559)).
* At the end of the program the library frees the hotkeys together with
  the commodity; the original left their memory allocated
  ([#557](https://github.com/fubar-coder/bb2-c/issues/557)).
* At the end of the program the library removes the commodity before it
  closes commodities.library. The original closed the library first and
  removed the commodity through it afterwards, which worked only while
  the library stayed in memory
  ([#556](https://github.com/fubar-coder/bb2-c/issues/556)).

{{% fixed-bugs-library part="elsewhere" %}}
