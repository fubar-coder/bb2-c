---
title: riregistrylib
weight: 36
description: The bugs of Blitz Basic 2 in the RWE userlib riregistrylib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in riregistrylib {#riregistrylib-fixes}

These fixes concern
[riregistrylib]({{< relref "/docs/libraries/userlibs/riregistrylib" >}}),
the interface of Red When Excited to registry.library, a settings
database that is not available any more; the fix of
[the text that follows its string functions]({{< relref "/docs/compared-with-original/fixed-bugs/libraries#rwe-text-after-string-function" >}})
is described for several libraries together. `LockRegistry` locks a registry structure and makes
it the current one, on which the other commands work; locks can be
nested, and `UnlockRegistry` returns to the one before.

* `UnlockRegistry` without a lock does nothing, and a 16th nested
  `LockRegistry` returns 0. The original had no limit in either
  direction: `UnlockRegistry` read and wrote below its list of locks,
  and a 16th lock was written over the library's own data. As
  registry.library cannot be found, `LockRegistry` locks nothing, so in
  the original every `LockRegistry` and `UnlockRegistry` pair went one
  entry lower, and the program crashed after a few of them
  ([#781](https://github.com/fubar-coder/bb2-c/issues/781)).
* `GetLibraryValue$` and `GetRegistryStringVariable` without a locked
  structure return ""; the original read the type and the value near
  address 0, which it did for every call without registry.library
  ([#780](https://github.com/fubar-coder/bb2-c/issues/780)).
* A program run again from the editor no longer works on the structure
  the run before had unlocked at its end
  ([#779](https://github.com/fubar-coder/bb2-c/issues/779)).
* `SetRegistryVariable name,value$` passes the length of the string as
  the size, `SetRegistryIntVariable` and `SetRegistryBoolVariable`
  pass 4; the original passed whatever value the program had left in a
  register ([#782](https://github.com/fubar-coder/bb2-c/issues/782)).

{{% fixed-bugs-library part="elsewhere" %}}
