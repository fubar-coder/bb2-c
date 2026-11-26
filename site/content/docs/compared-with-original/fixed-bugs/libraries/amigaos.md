---
title: AmigaOS calls
weight: 3
description: The bugs of Blitz Basic 2 in the calls of AmigaOS functions that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

These fixes concern the calls of AmigaOS functions
([AmigaOS libraries]({{< relref "/docs/libraries/amigaos" >}})).

## A missing AmigaOS library hung the program at its start {#missing-os-library}

A program opens every AmigaOS library it calls when it starts. When a
library could not be opened, the original tried again, without end. A
program that calls a function of a library the system does not have, such
as `gadtools.library` on Kickstart 1.3, hung before its first line, even
if it checks the system version and never makes that call.

**What happened**

On Kickstart 1.3, which has no `gadtools.library`:

```
DEFTYPE .l
NPrint "start"
Goto skip
x=CreateContext_(&x)
skip:
NPrint "end"
End
```

The original hangs at its start and prints nothing.

**With the port**

The program starts. The call of the missing library is never made, and
the program prints

```
start
end
```

A call of a function of the missing library would still crash.

Issue: [A program hung at its start if an AmigaOS library it calls was missing (#44)](https://github.com/fubar-coder/bb2-c/issues/44).

## Other fixes {#amigaos-other-fixes}

Each point says first what the original did and then what the port
does.

* A program that calls a resource, such as the battery clock or the CIA
  resources, hung at its start; the resource is opened
  ([#45](https://github.com/fubar-coder/bb2-c/issues/45)).
* `CachePreDMA_` and `CachePostDMA_` passed their flags in the wrong
  register; the flags reach the system
  ([#959](https://github.com/fubar-coder/bb2-c/issues/959)).
* A program that uses a transcendental function, such as `Sin`, `Sqr` or
  `^` on floats, hung at its start when `mathtrans.library` could not be
  opened, like the [missing AmigaOS libraries](#missing-os-library)
  above; the program starts, and only a call of such a function crashes
  ([#202](https://github.com/fubar-coder/bb2-c/issues/202)).
