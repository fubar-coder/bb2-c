---
title: speaklib
description: The bugs of Blitz Basic 2 in the command library speaklib that the
  BB2 C port fixes.
sidebar:
  order: 54
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/speaklib/
---

::fixed-bugs-library

## Changes for existing programs

### `Translate$` added a 0 byte to its result {#translate-length}

`Translate$` turns a text into phonemes, the form `PhoneticSpeak` speaks.

**What happened.** The original's result was one character too long: it
ended in the 0 byte that ends the phonemes in memory, and text added
after it came after that 0. This program prints the length and the last
character of the result, and of the result with an `x` added:

```
t$=Translate$("hello")
NPrint "len ",Len(t$)," last ",Asc(Right$(t$,1))
u$=Translate$("hello")+"x"
NPrint "len ",Len(u$)," 9th ",Asc(Mid$(u$,9,1))," 10th ",Asc(Mid$(u$,10,1))
End
```

**With the original**

```
len 10 last 0
len 11 9th 32 10th 0
```

**With the port**

```
len 9 last 32
len 10 9th 32 10th 120
```

The phonemes are `/HEH4LOW ` with a blank at the end, and the `x`
follows them.

**What to change.** A program that removed the last character of the
result itself, for example with `Left$(t$,Len(t$)-1)`, now removes a
real one: drop that step.

Issue: [Translate$ returns the length including the terminating 0 (#277)](https://github.com/fubar-coder/bb2-c/issues/277).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Sounds

Two more fixes of them change what an existing program does:
[`Filter On` switched the audio filter off](/docs/compared-with-original/libraries/commands/audiolib/#filter-on-off) and
[`Translate$` added a 0 byte to its result](#translate-length).

* `SetVoice` limited its values with a compare that overflowed, so a
  large negative rate, pitch or sample frequency became the highest
  value, for example a rate of -32768 the rate 400; such a value now
  becomes the lowest one
  ([#279](https://github.com/fubar-coder/bb2-c/issues/279)).
* When translator.library or narrator.device could not be opened, the
  end of the program still closed the device that was never opened and
  freed signal 0, which belongs to the system; it closes and frees only
  what was opened ([#278](https://github.com/fubar-coder/bb2-c/issues/278)).

::fixed-bugs-library{part="elsewhere"}
