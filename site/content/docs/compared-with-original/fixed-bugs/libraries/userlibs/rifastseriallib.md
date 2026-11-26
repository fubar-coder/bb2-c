---
title: rifastseriallib
weight: 22
description: The bugs of Blitz Basic 2 in the RWE userlib rifastseriallib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `SetIOSize` gave `DoSerialIO` one byte more {#setiosize-count}

[rifastseriallib]({{< relref "/docs/libraries/userlibs/rifastseriallib" >}})
links two Amigas through the serial port. `InitSerialIO` sets the
number of bytes that `DoSerialIO` without an argument exchanges with the
other machine, and `SetIOSize` changes it.

**What happened.** The original stored the count of `InitSerialIO` as
it is meant, but that of `SetIOSize` one too high: after

```
Dim inb.b(15),outb.b(15)
InitSerialIO 19200,4,&inb(0),&outb(0)
SetIOSize 16
DoSerialIO
```

`DoSerialIO` exchanged 17 bytes, one more than the buffers hold: it
waited for a 17th byte from the other machine, stored it behind `inb`
and sent the byte behind `outb`.

**With the port.** `SetIOSize n` makes `DoSerialIO` exchange n bytes,
as `InitSerialIO` with a count of n does.

**What to change.** A program that wrote `SetIOSize n-1` to get cycles
of n bytes now writes `SetIOSize n`. Both machines of a link must run
programs built with the same library: a machine that exchanges one byte
more than the other waits for a byte that never comes.

Issue: [SetIOSize stores its argument as it is, but DoSerialIO transfers one byte more: SetIOSize 16 runs cycles of 17 bytes (#1158)](https://github.com/fubar-coder/bb2-c/issues/1158).

## Changes you may notice

### The results of `InitSerialIO` and `GetSerial` {#fastserial-results}

`InitSerialIO` of
[rifastseriallib]({{< relref "/docs/libraries/userlibs/rifastseriallib" >}})
sets up the serial port for the library and does nothing when it is
already set up; `GetSerial address,bytes` waits for a block of bytes
from the other machine.

**What happened.** As functions, both returned one of their arguments:
`InitSerialIO` its baud rate, also when it did nothing, and `GetSerial
address,bytes` the low byte of its address. A program could not tell
from them whether anything had happened:

```
DEFTYPE .l
r1=InitSerialIO(19200,16,?inbuf,?outbuf)
r2=InitSerialIO(9600,16,?inbuf,?outbuf)
FinitSerialIO
NPrint r1," ",r2
End
Even
inbuf: Dcb.b 64,0
outbuf: Dcb.b 64,0
```

The original printed `19200 9600`, the port prints `-1 0`: the second
call does nothing, since the port is set up already.

**With the port.** `InitSerialIO` returns -1 when it set the port up and
0 when it did nothing, because the library is set up already or the
baud rate is outside 109 to 3546895. `GetSerial address,bytes` returns -1
when it received the block and 0 when it did nothing, before
`InitSerialIO` or for 0 bytes.

**What to change.** A program that used the result of `InitSerialIO` as
the baud rate keeps the rate in a variable of its own. Test the results
for -1 and 0.

Issues:

* [InitSerialIO returns its baud argument, also when it does nothing (#650)](https://github.com/fubar-coder/bb2-c/issues/650)
* [GetSerial address,bytes returns the low byte of its address argument (#649)](https://github.com/fubar-coder/bb2-c/issues/649)

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rifastseriallib {#rifastseriallib-fixes}

These fixes concern
[rifastseriallib]({{< relref "/docs/libraries/userlibs/rifastseriallib" >}}),
the fast serial links of Red When Excited. `SetIOSize` is described in
[Changes for existing programs](#setiosize-count), the results of
`InitSerialIO` and `GetSerial` in
[Changes you may notice](#fastserial-results). One bug of `SyncSerial`
stays ([Bugs not fixed yet]({{< relref "/docs/compared-with-original/bugs-not-fixed-yet" >}})).

* `InitSerialIO` and `SetIOSpeed` work on a 68000. The original used an
  instruction of the 68020 there and crashed
  ([#647](https://github.com/fubar-coder/bb2-c/issues/647)).
* A block of 0 bytes in `PutSerial`, `GetSerial`, `SyncPut` and `SyncGet`
  transfers nothing. The original took it for 4 GB
  ([#646](https://github.com/fubar-coder/bb2-c/issues/646)).
* `SetIOSpeed` takes the baud rate from its argument alone. The original
  also used the unused upper half of the register, and could set a wrong
  rate ([#654](https://github.com/fubar-coder/bb2-c/issues/654)).
* `FinitSerialIO`, also at the end of the program, switches the serial
  interrupts that were on before `InitSerialIO` back on, so serial.device
  works again afterwards. The original never switched them on, and wrote
  to address $9A in low memory instead of the chip register
  ([#651](https://github.com/fubar-coder/bb2-c/issues/651),
  [#648](https://github.com/fubar-coder/bb2-c/issues/648)).
* `InitSerialIO` sets the library up at every baud rate from 109 to
  3546895, and `InitSerialIO` and `SetIOSpeed` refuse another rate. With
  the original a rate of 1773448 or more left the library "not set up",
  so the later commands did nothing, and a rate of 0 divided by zero
  ([#653](https://github.com/fubar-coder/bb2-c/issues/653)).
* `DoSerialIO`, `GetSerial`, `SyncSerial`, `SyncPut`, `SyncGet` and
  `CheckSerial` do nothing before `InitSerialIO`. The original ran them
  on the port, `DoSerialIO` with buffers at address 0
  ([#652](https://github.com/fubar-coder/bb2-c/issues/652)).
* After `InitSerialIO` with a count of 0 or `SetIOSize 0`, `DoSerialIO`
  without an argument transfers nothing. The original exchanged 65536
  bytes after `InitSerialIO` with a count of 0, far past the buffers
  ([#1164](https://github.com/fubar-coder/bb2-c/issues/1164)).
* A program run again from the editor starts as the slave and with the
  library not set up. The original kept the state of the run before, so
  a program that never calls `SetSerialMode` could be the master
  ([#1159](https://github.com/fubar-coder/bb2-c/issues/1159)).

{{% fixed-bugs-library part="elsewhere" %}}
