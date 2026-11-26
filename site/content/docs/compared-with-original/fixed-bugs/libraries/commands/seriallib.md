---
title: seriallib
weight: 50
description: The bugs of Blitz Basic 2 in the command library seriallib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### The flags of `WriteSerialString` and `ReadSerial` {#serial-flags}

`WriteSerialString unit#,string,Flags` sends a string to a serial unit
opened with `OpenSerial`. Its help text names two flags: 1 makes the
command wait until the string is sent, and 2 sends the whole string,
zero bytes included, where without it the string ends at its first zero
byte. `ReadSerial(unit#,flags)` returns the next byte that came in, or
-1 if none is waiting; its help text says that the flag 1 makes it wait
for a byte instead.

**What happened.** The original took both meanings of
`WriteSerialString` from the flag 2 and never looked at the flag 1. A
program that passed 2 got the whole string and a command that waited; a
program that passed 1 got neither. `ReadSerial` waited only with the
flag 2, and with the flag 1 returned -1 at once when nothing had come.

**With the port.** Both commands take the flags of their help text:

```
WriteSerialString 0,a$,2   ; original: the whole string, waits until it is sent
                           ; port:     the whole string, goes on at once
WriteSerialString 0,a$,3   ; both:     the whole string, waits until it is sent
WriteSerialString 0,a$,1   ; original: up to the first zero byte, goes on at once
                           ; port:     up to the first zero byte, waits
c.w=ReadSerial(0,1)        ; original: -1 if no byte is waiting
                           ; port:     waits for the next byte
c.w=ReadSerial(0,2)        ; both:     waits for the next byte
```

**What to change.** A program that passes 2 to `WriteSerialString` and
counts on the command waiting passes 3; that works with the original and
the port alike. A write that does not wait is still going on when the
command returns: wait for it, with `Wait` and `SerialEvent`, before the
next command that writes to the same unit. A program that calls
`ReadSerial` with the flag 1 to look for a byte without waiting passes 0,
or leaves the flag out.

Issues:

* [WriteSerialString took TrueString and DoIO both from the flag 2 (#254)](https://github.com/fubar-coder/bb2-c/issues/254)
* [ReadSerial waited with the flag 2, not 1 (#255)](https://github.com/fubar-coder/bb2-c/issues/255)

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Serial port

These are the commands of the serial port library, which open units of
`serial.device` and read and write through them. Two more fixes of them
change what an existing program does:
[the flags of `WriteSerialString` and `ReadSerial`](#serial-flags).

* `CloseSerial`, `SetSerialBuffer`, `SetSerialLens`, `SetSerialParams`
  and `SerialEvent` looked for a unit other than 0 at half its place,
  in the data of another unit, so `CloseSerial 1` with unit 0 open closed
  unit 0; every command finds each unit at its own place
  ([#252](https://github.com/fubar-coder/bb2-c/issues/252)).
* `ReadSerialMem` read through the request the unit uses for writing,
  so the read settings of `SetSerialParams`, such as the end of file
  mode, did not apply to it; it reads through the unit's read request
  ([#253](https://github.com/fubar-coder/bb2-c/issues/253)).
* `ReadSerialString` read every byte waiting, also more than its string
  buffer holds, and overwrote the memory behind the buffer; it reads at
  most what fits, and the rest stays waiting for the next read
  ([#256](https://github.com/fubar-coder/bb2-c/issues/256)).
* `CloseSerial` closed the device while a write that does not wait was
  still going on, and left that write pending; it stops and collects the
  write and the read before it closes the device
  ([#258](https://github.com/fubar-coder/bb2-c/issues/258)).
* When `OpenSerial` got no free signal for writing, it returned 0 but
  left `serial.device` open, so the unit could not be opened again until
  a reboot; it closes the device again
  ([#259](https://github.com/fubar-coder/bb2-c/issues/259)).
* A second `OpenSerial` of a unit that was open set the unit up again;
  when the device then refused the second open, the unit counted as
  closed and the device was never closed. A second `OpenSerial` of an
  open unit returns 0 and leaves the unit as it is, also with shared
  access (the flag 32), where the original opened the unit again over the
  first one
  ([#260](https://github.com/fubar-coder/bb2-c/issues/260)).

{{% fixed-bugs-library part="elsewhere" %}}
