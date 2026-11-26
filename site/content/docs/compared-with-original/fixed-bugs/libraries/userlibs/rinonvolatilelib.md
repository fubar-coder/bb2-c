---
title: rinonvolatilelib
weight: 34
description: The bugs of Blitz Basic 2 in the RWE userlib rinonvolatilelib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `StoreNVData` returned 0 for a stored item {#storenvdata-result}

`StoreNVData` of
[rinonvolatilelib]({{< relref "/docs/libraries/userlibs/rinonvolatilelib" >}})
stores an item, such as a saved game, in the nonvolatile memory of a
CD32 or in its replacement on disk. Its guide documents the result as
True or False, whether the item was stored.

**What happened.** The original returned the result of the system's
nonvolatile.library instead: 0 when the item was stored, and an error
code when it was not. A program that tested the result as documented
reported every successful save as a failure, and every failed one as a
success. This program stores a long on a system where the item cannot
be stored, as in the emulator, which has no nonvolatile memory and only
a read-only system disk:

```
d.l=$12345678
NPrint "open ",IsNonvolatile
r.w=StoreNVData("RWETest","Item",&d,4)
If r Then NPrint "stored" Else NPrint "not stored"
NPrint "result ",r
End
```

**With the original**

```
open -1
stored
result 3
```

**With the port**

```
open -1
not stored
result 0
```

With the port, `StoreNVData` returns -1 when the item was stored and 0
when it was not, also without nonvolatile.library.

**What to change.** A program that tested for 0 to find a successful
save has to test for a value other than 0, as the guide says:
`If StoreNVData(...)` is true when the item was stored.

Issue: [StoreNVData returns StoreNV's result (0 = stored) instead of the documented TRUE/FALSE (#1085)](https://github.com/fubar-coder/bb2-c/issues/1085).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rinonvolatilelib {#rinonvolatilelib-fixes}

These fixes concern
[rinonvolatilelib]({{< relref "/docs/libraries/userlibs/rinonvolatilelib" >}}),
the access to the system's nonvolatile.library by Reflective Images; the
fix of [the result of `StoreNVData`](#storenvdata-result) is described
above.

* `StoreNVData` stores an item of 655360 bytes or more with its whole
  length; the original passed a wrong, much smaller length to the
  system, and 655365 bytes were stored as 60 bytes without an error
  ([#764](https://github.com/fubar-coder/bb2-c/issues/764)).

{{% fixed-bugs-library part="elsewhere" %}}
