---
title: audiolib
weight: 6
description: The bugs of Blitz Basic 2 in the command library audiolib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/audiolib/]
---

{{< fixed-bugs-library >}}

## Changes you may notice

### `Filter On` switched the audio filter off {#filter-on-off}

The Amiga has a low-pass filter on its audio output that softens the
sound, and `Filter On` and `Filter Off` are meant to switch it on and
off. The filter is controlled by bit 1 of port A of the CIA-A chip, the
same bit that sets the power light: a 0 switches the filter on and makes
the light bright, a 1 switches the filter off and dims the light.

**What happened.** The original had the two the wrong way round: `Filter
On` set the bit and so switched the filter off, and `Filter Off` switched
it on. This program reads the bit after each command:

```
Filter 1 : f1=Peek.b($BFE001) AND 2
Filter 0 : f0=Peek.b($BFE001) AND 2
NPrint "filter ",f1," ",f0
```

**With the original**

```
filter 2 0
```

**With the port**

```
filter 0 2
```

`Filter On` clears the bit, so the filter is on and the power light
bright, as the manual says; `Filter Off` sets it.

**What to change.** Every program that uses `Filter` now gets the other
filter state. A program that was written for the sound of the original,
with `Filter Off` for a soft sound or `Filter On` for a bright one, swaps
the two. A program that followed the manual needs no change: it now gets
the filter it asked for.

Issue: [Filter On switches the audio low-pass filter off (and dims the power LED), Filter Off switches it on (#985)](https://github.com/fubar-coder/bb2-c/issues/985).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Sounds

Two more fixes of them change what an existing program does:
[`Filter On` switched the audio filter off](#filter-on-off) and
[`Translate$` added a 0 byte to its result]({{< relref "/docs/compared-with-original/libraries/commands/speaklib#translate-length" >}}).

* `DecodeSound` uses sound data that lies in chip memory where it is, and
  marks the sound so that this memory is not freed. The mark stayed when
  the next sample was put into the same sound object, by `DecodeSound` of
  other memory, `LoadSound` or `InitSound`, so that sample was never freed
  before the program ended; it is freed when the object is reused or
  freed ([#68](https://github.com/fubar-coder/bb2-c/issues/68)).
* `DiskPlay` of a file whose sample length is a multiple of the
  `DiskBuffer` length, or 0, never returned and played its last buffer
  again and again; it returns after the sample, as for any other length
  ([#69](https://github.com/fubar-coder/bb2-c/issues/69)).
* `LoadSound` and `DiskPlay` left the file open when they stopped with
  "Can't Find VHDR", "Error Reading File" or "Sound too long", so it
  stayed locked until the next reboot; the file is closed before the
  error ([#67](https://github.com/fubar-coder/bb2-c/issues/67)).
* `DiskPlay` of a file that cannot be opened or read kept its buffer of
  twice the `DiskBuffer` length in chip memory, so a program that caught
  the error with `SetErr` lost that memory each time; the buffer is freed
  ([#950](https://github.com/fubar-coder/bb2-c/issues/950)).

#### The check of `SetPeriod` {#setperiod-check}

**What happened.** With runtime error checking, `SetPeriod Sound#,Period`
tested the period as if it were a mask of audio channels, which runs
from 0 to 15. Every period above 15, and so every one a sound is played
with, stopped the program with "Illegal Chanel Mask":

```
InitSound 0,32
NPrint "before"
SetPeriod 0,428
NPrint "after"
End
```

The original printed `before` and stopped with the error.

**With the port.** The check tests only that the sound is set up, and a
sound that is not is still "Uninitialized Sound". The program above
prints `before` and `after`. Without runtime error checking nothing
changes.

**What to change.** Nothing.

Issue: [SetPeriod: error check tests the period as a channel mask (#66)](https://github.com/fubar-coder/bb2-c/issues/66).

{{% fixed-bugs-library part="elsewhere" %}}
