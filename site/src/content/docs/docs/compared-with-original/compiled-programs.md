---
title: Compiled programs
description: Fixed bugs of Blitz Basic 2 in the program the compiler generates -
  AmigaOS functions and objects in the generated code, the end of a program,
  library calls and the executable file.
sidebar:
  order: 4
prev:
  link: /docs/compared-with-original/expressions/type-system/
  label: Expression type system
---

These bugs of the original compiler were in the program it generated. The
source compiled without a message, but the program computed a wrong
value, crashed or hung when it ran, or the executable file was not what
it should be: the code around an AmigaOS function or an object inside a
calculation, the code that ends a program, the calls of libraries in an
assignment, and the executable itself. The first sections show the ones
an ordinary program can run into, each with a short program. The other
fixes follow in tables by topic.

How the compiler reads a source is on
[Language](/docs/compared-with-original/language/), and
what it makes of an expression on
[Expressions](/docs/compared-with-original/expressions/).
The table on [Important Fixes & Breaking Changes](/docs/compared-with-original/important-fixes/)
lists all fixed bugs by what they can change in a program.

The outputs shown were recorded in an emulator. The original is Blitz
Basic 2 built from its published source, with the libraries of the Blitz
Basic 2 CD.

## AmigaOS functions and objects

### An AmigaOS function inside a calculation {#amigaos-call-in-expression}

While the compiler works out a calculation, it keeps the values it has
already loaded in the processor's registers. An AmigaOS function such as
`Write_` takes its arguments in registers too. When the function came in
the middle of a calculation, the original loaded the arguments into
those registers without saving what was there, so the calculation went
on with an argument in place of one of its own values. No message
warned about it, and the result was simply wrong.

**What happened**

```
fh.l=Open_("NIL:",1006)
s$="abcd"
a.l=1000:b.l=200:c.l=30
x.l=Write_(fh,&s$,4)
NPrint a+b+c+x
y.l=a+(b+(c+Write_(fh,&s$,4)))
If y=1234 Then NPrint y Else NPrint "wrong: not 1234"
Close_ fh
End
```

`Write_` writes the 4 bytes and returns 4, so both sums are 1234. The
first adds the result of a separate call; the second calls `Write_` in
the middle of the calculation. The original prints

```
1234
wrong: not 1234
```

In the second sum, the value of `c` was overwritten by an argument of
`Write_`. Functions with many arguments in registers, such as
`BltBitMap_`, could overwrite several values of a calculation at once,
and a string calculation could lose its text to a function with an
argument in register a3, such as `RawDoFmt_`.

**With the port**

The compiler saves the values of the calculation before the call and
restores them after it. The program prints

```
1234
1234
```

**What to change**

Nothing. A program that called the function on a line of its own, as
in the first sum, to avoid the bug can stay as it is.

Issue: [An AmigaOS function used inside an expression overwrote the expression's values (#344)](https://github.com/fubar-coder/bb2-c/issues/344).

### Object numbers above a raised maximum {#object-numbers-above-32k}

Objects such as shapes and bitmaps are numbered, and the compiler options
set the highest number a program may use (**Object Maximums**). Each
object has an entry in a table of its kind, and the compiler finds an
object from its number and the size of an entry. The original computed
this distance with a 16-bit number, which holds at most 32767. When a
maximum is raised so far that an object lies 32 KB or more behind the
start of its table, the result turned negative. For shapes this happens
from Shape 1024 on, for bitmaps from BitMap 512 on. `Addr`, `Use`, `Free`
and the object number of a command such as `BitMap 512,…` then addressed
memory before the table instead of the object. Runtime error checking did not notice it, since the number
itself was within the maximum.

**What happened**

This program is compiled with the maximum of Shapes raised to 1100:

```
DEFTYPE .l
a.l=Addr Shape(1025)-Addr Shape(0)
NPrint a
End
```

An entry of the shape table has 32 bytes, so Shape 1025 lies 32800 bytes
behind Shape 0. The original prints

```
-32736
```

**With the port**

When the last object of a maximum lies 32 KB or more behind the start of
its table, the compiler computes the distance with a 32-bit number. The
program prints

```
32800
```

With smaller maxima, among them all the defaults, the compiler generates
the same code as the original.

`Used`, which gives the number of the object in use, also computed it
with a 16-bit number and gave a wrong number for an object 64 KB or more
behind the start of its table, such as 1 for BitMap 1025; with such a
maximum it gives the right number
([#998](https://github.com/fubar-coder/bb2-c/issues/998)).

**What to change**

Nothing. `LoadShapes`, `DecodeShapes` and `LoadSprites`, which load a
whole range of objects up to the maximum, had the same fault in their
libraries; they load the whole range with such a maximum as well
([Shapes and sprites](/docs/compared-with-original/libraries/commands/shapeslib/#shapes-and-sprites)).

Issues: [Objects past 32 KB of their table were addressed before the table (#939)](https://github.com/fubar-coder/bb2-c/issues/939),
[Used Object returned a wrong number from 64 KB of its table on (#998)](https://github.com/fubar-coder/bb2-c/issues/998).

### A function of an object inside a string {#object-function-in-string}

Some library functions take their object in a special register, which is
also where the compiler keeps the end of a string while it builds it:
`BlitColl` (a shape), `PeekSound` (a sound) and others. Used inside a
string expression, the original wrote the rest of the string into the
object.

**What happened**

```
BitMap 0,64,32,2 : Use BitMap 0
GetaShape 1,8,4,20,9
BitMap 1,96,48,3 : Use BitMap 1
NPrint "a ",BlitColl(1,40,20)," b"
NPrint ShapeWidth(1)
End
```

The original prints `8290` as the width of the shape: the characters
`" b"` were written over it. The shape was 20 pixels wide, and the
damaged shape can crash the program when it is freed at the end.

**With the port**

The object stays as it is; the program prints `20`.

Issue: [PeekSound in a string expression overwrites the sound object (#46)](https://github.com/fubar-coder/bb2-c/issues/46).

## Ending a program

### A program with the Blitz OS commands hung at its end {#os-commands-end}

The Blitz OS commands, `InitPage` and the others, draw with the shapes
library. A program that uses them, but no shapes command of its own, did
not get the memory for its shapes: at `End` the program then freed 100
shapes from address 0, which corrupts the system's memory. The program
did not even have to run one of these commands; using one anywhere in the
source was enough.

**What happened**

```
NPrint "x"
End
InitPage 0,0,2
```

The original prints

```
x
```

and then the system hangs, although `InitPage` behind `End` is never
reached. A shapes command somewhere in the program, for example
`LoadShape 0,"x"` behind `End`, avoided the hang.

**With the port**

The memory for the shapes is reserved. The program prints `x` and ends
normally, with return code 0.

**What to change**

Nothing. A shapes command that a program contains only to avoid the hang
is no longer needed and can be removed.

Issue: [A program could hang the system at its end when it used a library that needs objects only through another library (#335)](https://github.com/fubar-coder/bb2-c/issues/335).

### Data of odd length at the end of a program {#odd-end}

The compiler puts the code that ends a program behind the last statement
of the source. When that statement left an odd number of bytes - a
`Dc.b`, `Dcb.b`, `Ds.b` or `IncBin` of odd length - the original placed
this code at an odd address, where no 68000-family CPU can run code. The
program then crashed at its end, with a "Software Failure" requester:

```
NPrint "before"
End          ; original: crashes here ("Software Failure")    port: ends normally
Dc.b "abc"
```

The port starts the end code at an even address, and the program ends
normally.

Issue: [A program ending with data of odd length crashed at its end (#979)](https://github.com/fubar-coder/bb2-c/issues/979).

## Other fixes

The fixes below need rarer conditions. Each row of a table says what the
original did and what the port does, and links the issue with the
details.

### Assignments and library calls {#library-calls}

| What was wrong | Now | Issue |
|---|---|---|
| An AmigaOS function (`Name_`) after a Float calculation in the same expression made the Float operations after it go through that library: `Abs(-2)+FindTask_(0)` crashed and `x*y+IEEESPFix_(i)` gave a wrong value. | The Float operations use their own library. An AmigaOS function before the Float part was not affected. | [#1211](https://github.com/fubar-coder/bb2-c/issues/1211) |
| In an assignment to an array element whose index calls a library function such as `Abs`, the original assumed on the right side that this function's library was already loaded, although the right side runs first: `a(Abs(-j)) = Abs(-5)` could store -5 or crash, depending on what ran before. | The right side loads the library itself. | [#1212](https://github.com/fubar-coder/bb2-c/issues/1212) |
| In an assignment to a string array element whose index works with strings, the index overwrote the string about to be stored: `s$(Val(d$)) = "ab"+c$` with `d$="2"` stored `2`, a zero byte and `cde` instead of `abcde`. This happened with string functions such as `Val`, `Mid$` or `Instr`, a string comparison, or a Function that works with strings in the index, also for a string field of a NewType array. | The string is stored as it was computed. | [#1213](https://github.com/fubar-coder/bb2-c/issues/1213) |
| In an assignment to a number array element whose index works with strings, a right side that builds a string, such as `a(Val(d$)) = Len(c$+"x")`, built it wherever the string pointer happened to point. The value stored was right, but after `Read t$` this overwrote the next `Data` item, and the next `Read` crashed. | The right side builds its string in the string work area. | [#1214](https://github.com/fubar-coder/bb2-c/issues/1214) |

### The executable

| What was wrong | Now | Issue |
|---|---|---|
| Calls of AmigaOS functions with an argument in register a4 or a5 crashed or destroyed the program's variables. | The compiler saves both registers around such a call. | [#958](https://github.com/fubar-coder/bb2-c/issues/958) |
| Very large executables, with very many relocations, did not load on AmigaOS 2.0 and later. | Their relocation table is split into groups AmigaOS accepts. | [#943](https://github.com/fubar-coder/bb2-c/issues/943) |
| Pad bytes, from `Even`, from the alignment of data and code and from `Ds`, held random values. | They are zero. | [#969](https://github.com/fubar-coder/bb2-c/issues/969) |
| When the code of a program had an odd number of words, CREATE EXECUTABLE filled the last word of the code with 2 bytes from behind the compiler's own memory. On a plain system these bytes are 0; under a memory checker or a similar tool that fills the memory behind each allocation, the same source gave a different executable from compile to compile. The program ran the same either way. | The compiler writes this word as zero, also in AB3 mode. | [#1184](https://github.com/fubar-coder/bb2-c/issues/1184) |
| On a 68010 processor, runtime errors and the debugger took the trap vectors from the wrong table when the system had moved it. | The moved table is used. | [#42](https://github.com/fubar-coder/bb2-c/issues/42) |
| `TokeJsr` in a source and `ALibJsr` in a library call the routine of a library command. Without runtime error checking, such a call to a routine that has an error check went through an extra stub, a `jsr` to the routine and an `rts`: 8 bytes and one extra call each, for a check that is not part of the program. | The call goes straight to the routine. | [#1048](https://github.com/fubar-coder/bb2-c/issues/1048) |
| An operation of two constants, such as `7 Mod 3`, is worked out while compiling. When it was the first use of a library, such as the one of `Mod`, the addresses in that library's code were left out of the list AmigaOS uses to adjust addresses when it loads the program: a later `Mod` of Long values jumped into the compiler's memory and worked only while that memory was unchanged. | The addresses are in the list. | [#1049](https://github.com/fubar-coder/bb2-c/issues/1049) |
