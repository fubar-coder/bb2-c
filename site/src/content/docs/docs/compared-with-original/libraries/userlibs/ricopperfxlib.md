---
title: ricopperfxlib
description: The bugs of Blitz Basic 2 in the RWE userlib ricopperfxlib that the
  BB2 C port fixes.
sidebar:
  order: 14
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/ricopperfxlib/
---

::fixed-bugs-library

## Changes for existing programs

### `GetCCOffset` returned -1 after `CopperResetAdr` {#getccoffset-resetadr}

`GetCCOffset` of
[ricopperfxlib](/docs/libraries/userlibs/ricopperfxlib/)
returns the place in the custom space where the library writes its next
copper instruction, counted in instructions; `RedoColSplit` needs it to
find a colour split again. `CopperResetAdr` points the library at a
space in a copper list of the program's own, instead of the custom space
of a coplist.

**What happened.** The original's `GetCCOffset` tested whether a
coplist had been chosen, not whether there was a custom space, so after
`CopperResetAdr` it returned -1, "no custom space", although the other
commands wrote their instructions. With a block of memory at `m`:

```
CopperResetAdr m,44
CopperMove $180,$fff
NPrint GetCCOffset
```

With the original this printed `-1`; with the port it prints `1`, the
place after the `CopperMove`.

**What to change.** A program that used `RedoColSplit` with
`CopperResetAdr` and counted the offsets itself can take them from
`GetCCOffset`.

Issue: [GetCCOffset returns -1 after CopperResetAdr although the commands work (#593)](https://github.com/fubar-coder/bb2-c/issues/593).

### `DoColSplit` and `CopperAGACol` returned no useful value {#rwe-effect-results}

`DoColSplit` of
[ricopperfxlib](/docs/libraries/userlibs/ricopperfxlib/)
writes a colour split into the custom space of a coplist, and
`CopperAGACol` writes one AGA colour there. Both can also be used as
functions: they return -1 when they wrote their instructions and 0 when
no custom space was chosen with `CopperReset` or `CopperResetAdr`.

**What happened.** The original set no result for `DoColSplit`: the
program got whatever the command had left in the register, the address
of its colour table or a colour step of the gradient. `CopperAGACol`
returned 1234 without a custom space and its colour number with one.

**What to change.** A program that uses the commands as statements
works as before. One that used the colour number `CopperAGACol`
returned takes it from its own variables.

Issues:

* [DoColSplit is declared with a word result but returns no defined value (#592)](https://github.com/fubar-coder/bb2-c/issues/592)
* [CopperAGACol is declared with a long result but returns 1234 or its col# argument (#590)](https://github.com/fubar-coder/bb2-c/issues/590)

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ricopperfxlib {#ricopperfxlib-fixes}

These fixes concern
[ricopperfxlib](/docs/libraries/userlibs/ricopperfxlib/),
the copper instructions by Reflective Images for the custom space of a
coplist. The fixes of [`GetCCOffset` after `CopperResetAdr`](#getccoffset-resetadr)
and of [the results of `DoColSplit` and `CopperAGACol`](#rwe-effect-results)
are described above.

* A colour split of `DoColSplit` that starts at line 0 starts there. The
  original first wrote the wait for line 256 that the copper needs for
  the lines below 255, so the split appeared at line 256, or not at all
  on a display that ends before it
  ([#588](https://github.com/fubar-coder/bb2-c/issues/588)).
* `CopperWait` with an `x` of 500 or more waits on the line given. The
  original waited one line later
  ([#591](https://github.com/fubar-coder/bb2-c/issues/591)).
* `RedoColSplit` with a `ccoffset` of 16384 instructions or more writes
  its colours at that place. The original wrote them 64 KB or a multiple
  of it away, into other memory. `GetCCOffset` never returns such a
  large offset; only a program that computes the offset itself meets it
  ([#594](https://github.com/fubar-coder/bb2-c/issues/594)).
* A program run a second time from the editor starts with no custom
  space chosen and with the `ColSplitBplcon3` value 0. In the original,
  every command before `CopperReset` wrote into the copper list of the
  run before, which had been freed, and that run's `ColSplitBplcon3`
  value stayed in effect
  ([#1126](https://github.com/fubar-coder/bb2-c/issues/1126)).

#### A colour split of fewer than one line {#colsplit-numlines}

`DoColSplit` of
[ricopperfxlib](/docs/libraries/userlibs/ricopperfxlib/)
writes a colour gradient over `numlines` lines into the custom space of
a coplist, and `RedoColSplit` writes new colours into such a split.

**What happened.** The original did not check `numlines`, also not with
runtime error checking. A count of 0 divided by zero and stopped the
program with a processor exception; a negative count wrote up to 65536
lines of copper instructions over the memory behind the custom space.

**With the port.** With runtime error checking, a `numlines` below 1 is
the error "Colour split needs at least one line". Without runtime error
checking, the commands work as in the original.

**What to change.** Give every colour split at least one line.

Issue: [DoColSplit/RedoColSplit do not check numlines: 0 divides by zero, a negative value writes up to 65536 lines (#589)](https://github.com/fubar-coder/bb2-c/issues/589).

::fixed-bugs-library{part="elsewhere"}
