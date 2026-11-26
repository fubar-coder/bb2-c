---
title: riseriallib
weight: 38
description: The bugs of Blitz Basic 2 in the RWE userlib riseriallib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/riseriallib/]
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `SerialGot` returned -1 instead of the number of bytes {#serialgot-count}

[riseriallib]({{< relref "/docs/libraries/userlibs/riseriallib" >}})
drives the serial port with interrupts of its own and keeps the bytes
received in a buffer. Its help text says that `SerialGot` "returns num
chars in serial buffer".

**What happened.** The original's `SerialGot` returned -1 when at least
one byte was waiting and 0 when none was. After the other machine had
sent the 16 bytes "0123456789ABCDEF",

```
SerialOpen 19200
; ... the other machine sends 16 bytes
NPrint SerialGot
```

printed `-1`.

**With the port.** `SerialGot` returns the number of bytes waiting in
the buffer, 0 to 255; the lines above print `16`.

**What to change.** A program that tested `If SerialGot` or `SerialGot<>0`
works as before. One that compared the result with -1, such as
`If SerialGot=-1`, tests for a value other than 0 instead.

Issue: [SerialGot returns -1/0, not the number of bytes in the buffer its help text promises (#802)](https://github.com/fubar-coder/bb2-c/issues/802).

## Changes you may notice

### The block commands of riseriallib {#serial-block-commands}

`SerialSendBlock address,length` of
[riseriallib]({{< relref "/docs/libraries/userlibs/riseriallib" >}})
sends a block of bytes through the serial port, `SerialReceiveBlock
address,length` waits for one, and `SerialFlush` empties the buffer of
bytes received.

**What happened.** The three commands were meant to open the port when
it was closed, as the string and long commands do, but a wrong test
kept them from it. `SerialSendBlock` before `SerialOpen` sent nothing,
and `SerialReceiveBlock` waited for bytes the closed port never
received. `SerialReceiveBlock` also first threw away the bytes already
in the buffer, so bytes the other machine had sent before the call were
lost, and the call waited for them for ever.

**With the port.** `SerialSendBlock`, `SerialReceiveBlock` and
`SerialFlush` open the port when it is closed, and it stays open
afterwards, also after `SerialFlush`. `SerialReceiveBlock` keeps the
bytes waiting in the buffer: they are the first bytes of the block.

**What to change.** A program that wants a block of only the bytes
that arrive after the call calls `SerialFlush` before
`SerialReceiveBlock`. A program that called `SerialFlush` with the port
closed and needs it closed afterwards calls `SerialClose`.

Issues:

* [The word open flag is tested as a long: SerialSendBlock, SerialReceiveBlock and SerialFlush never open the port (#792)](https://github.com/fubar-coder/bb2-c/issues/792)
* [SerialReceiveBlock throws away the bytes already received (#798)](https://github.com/fubar-coder/bb2-c/issues/798)

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in riseriallib {#riseriallib-fixes}

These fixes concern
[riseriallib]({{< relref "/docs/libraries/userlibs/riseriallib" >}}),
the serial port with interrupts of its own. `SerialGot` is described in
[Changes for existing programs](#serialgot-count), the block commands
in [Changes you may notice](#serial-block-commands), and text after
`SerialReadString` in
[Text after a string function]({{< relref "/docs/compared-with-original/libraries#text-after-string-function" >}}).

* `SerialClose` switches only the library's two serial interrupts off
  and back to their state at `SerialOpen`. The original switched every
  interrupt off and then on those that were on at `SerialOpen`, so an
  interrupt switched on since was lost and one switched off since came
  back
  ([#790](https://github.com/fubar-coder/bb2-c/issues/790)).
* `SerialRead` and `SerialWait` return the byte alone, 0 to 255, or -1
  for none. The original left the upper bits of the result as they were
  before the call, so a byte could read as any large number or as -1
  ([#797](https://github.com/fubar-coder/bb2-c/issues/797)).
* `SerialSendBlock` with a length of 0 or less sends nothing. The
  original sent one byte
  ([#800](https://github.com/fubar-coder/bb2-c/issues/800)).
* `SerialReceiveBlock` with a length of 1 receives one byte. The original
  never ended and wrote past the buffer
  ([#799](https://github.com/fubar-coder/bb2-c/issues/799)).
* `SerialStop` during `SerialReceiveBlock` ends the block after the next
  byte, and the program goes on after the command. The original returned
  to a wrong address
  ([#803](https://github.com/fubar-coder/bb2-c/issues/803)).
* A `SerialStop` while no block is sent or received no longer cuts the
  next block short. The original stopped the next `SerialSendBlock` after
  its first byte
  ([#1162](https://github.com/fubar-coder/bb2-c/issues/1162)).
* `SerialSendString` sends the characters of the string, without a 0
  byte behind them; send one with `SerialWrite 0` if the other machine
  reads with `SerialReadString`. The original sent the low byte of the
  string's address
  ([#801](https://github.com/fubar-coder/bb2-c/issues/801)).
* `SerialSendString` and `SerialReadString` open the port when it is
  closed. The original did so only when it was open: with the port
  closed it sent nothing, or waited for ever
  ([#793](https://github.com/fubar-coder/bb2-c/issues/793)).
* `SerialReadString` with the port open returns to the program. The
  original returned to a wrong address
  ([#796](https://github.com/fubar-coder/bb2-c/issues/796)).
* `SerialReadString` stops at the end of the string work area, 10240
  bytes unless the compiler options set another size, and leaves the
  rest in the buffer. The original wrote on past it until a 0 byte came
  ([#1161](https://github.com/fubar-coder/bb2-c/issues/1161)).
* `SerialReadLong` reads four bytes, the low byte first, and returns the
  long. The original never returned
  ([#794](https://github.com/fubar-coder/bb2-c/issues/794)).
* `SerialWriteLong` sends the four bytes of its argument, the low byte
  first. The original sent the low byte and then three times the byte 1
  ([#805](https://github.com/fubar-coder/bb2-c/issues/805)).
* When more than 255 bytes arrive unread, the first 255 stay in the
  buffer and the others are dropped. With the original the buffer then
  counted as empty, and all of them were lost
  ([#1160](https://github.com/fubar-coder/bb2-c/issues/1160)).
* `SerialWrite`, and so `SerialSendBlock`, `SerialSendString` and
  `SerialWriteLong`, waits for room while the transmit buffer is full
  and the port is open, so a long block arrives complete. The original
  wrote over bytes not sent yet, and a block of more than about 128 bytes
  lost some of them
  ([#983](https://github.com/fubar-coder/bb2-c/issues/983)).
* `SerialOpen` installs its interrupts in the vector table the processor
  uses, also when a 68010 or later has it moved. The original wrote them
  at the fixed addresses in low memory, where they were then never called
  ([#804](https://github.com/fubar-coder/bb2-c/issues/804)).

{{% fixed-bugs-library part="elsewhere" %}}
