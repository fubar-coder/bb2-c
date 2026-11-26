---
title: ridisasmlib
weight: 17
description: The bugs of Blitz Basic 2 in the RWE userlib ridisasmlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/ridisasmlib/]
---

{{< fixed-bugs-library >}}

## Changes you may notice

### `DisAssemble$` cut long lines after 34 columns {#disassemble-line-length}

`DisAssemble$(address)` of
[ridisasmlib]({{< relref "/docs/libraries/userlibs/ridisasmlib" >}})
returns the instruction at an address as a line of assembler text: the
name of the instruction in the first 9 columns, its operands behind
them, and spaces up to a length of 34 characters.

**What happened.** The original cut every line after 34 columns. Most
lines fit, but the operands of long 68020 instructions were cut in the
middle. The longest of them also ran over the end of the library's line
buffer and overwrote its own variables. This program disassembles a
`MOVE.l` with two memory indirect operands, and then an `RTS`:

```
DEFTYPE .l
Dim c.w(12)
c(0)=$21B0 : c(1)=$0133 : c(2)=$8000 : c(3)=$0000 : c(4)=$8000 : c(5)=$0000
c(6)=$0133 : c(7)=$8000 : c(8)=$0000 : c(9)=$8000 : c(10)=$0000
a$=DisAssemble$(&c(0))
NPrint Len(a$)," [",a$,"] ",InstrSize
c(0)=$4E75
a$=DisAssemble$(&c(0))
NPrint Len(a$)," [",a$,"] ",InstrSize
End
```

**With the original**

```
34 [move.l   ([-2147483648,a0,d0.w],-2] 22
34 [rts                               ] 2
```

**With the port**

```
80 [move.l   ([-2147483648,a0,d0.w],-2147483648),([-2147483648,a0,d0.w],-2147483648)] 22
34 [rts                               ] 2
```

**What to change.** A line is at least 34 characters long, and a longer
one comes back whole. A program that shows the lines in a column of 34
characters takes `Left$(a$,34)`.

Issue: [Long lines are cut after 34 columns, and a line can run over the end of the 80-byte LineBuff (#616)](https://github.com/fubar-coder/bb2-c/issues/616).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ridisasmlib {#ridisasmlib-fixes}

These fixes concern
[ridisasmlib]({{< relref "/docs/libraries/userlibs/ridisasmlib" >}}),
the 680x0 disassembler by Leading Edge Software. The fix of
[long lines](#disassemble-line-length) is described above.

* `DisAssemble$` shows a displacement or offset of 0 as `0`. The
  original showed the bytes at memory address 8 instead, which are often
  nothing, so `0(a0)` came out as `(a0)`
  ([#614](https://github.com/fubar-coder/bb2-c/issues/614)).
* `DisAssemble$` shows a `MOVEM` with an empty register list without a
  stray comma, as `movem.l  -(a7)`; the original wrote
  `movem.l  ,-(a7)`
  ([#615](https://github.com/fubar-coder/bb2-c/issues/615)).
* `DisAssemble$` no longer stops a 68000 with an address error when it
  follows a string of odd length in an expression, as in
  `"x"+DisAssemble$(a)`
  ([#617](https://github.com/fubar-coder/bb2-c/issues/617)).
* `InstrSize` is 0 before the first `DisAssemble$` of a run, also in a
  second run from the editor. The original returned the size of the run
  before ([#1170](https://github.com/fubar-coder/bb2-c/issues/1170)).

{{% fixed-bugs-library part="elsewhere" %}}
