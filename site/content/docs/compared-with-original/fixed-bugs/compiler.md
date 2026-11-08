---
title: Compiling
weight: 1
description: Fixed bugs of Blitz Basic 2 in what the compiler makes of a source, with examples of the ones ordinary programs meet.
---

These bugs of the original compiler changed what a source compiled to: a
wrong result, a program that crashed, or an error message where none
belonged. The first sections show the ones an ordinary program can run
into, each with a short program. The other fixes follow as a list, one
sentence each with a link to its issue.

The outputs shown were recorded in an emulator. The original is Blitz
Basic 2 built from its published source, with the libraries of the Blitz
Basic 2 CD.

## Operators and numbers

### `=<` meant "not equal" {#less-or-equal}

Blitz Basic 2 has two spellings for each comparison that includes
equality: `<=` and `=<` for less-or-equal, `>=` and `=>` for
greater-or-equal. Every one of them worked except `=<`, which the
original compiled as "not equal", without any message.

**What happened**

```
a=1:b=2:NPrint a=<b
a=2:b=2:NPrint a=<b
a=3:b=2:NPrint a=<b
x.f=1.5:y.f=1.5:NPrint x=<y
NPrint 3=<2
For i=1 To 3
  If i=<2 Then NPrint "le ",i Else NPrint "gt ",i
Next
End
```

The original prints the results of `<>`: true (-1) where the two values
differ, false (0) where they are equal.

```
-1
0
-1
0
-1
le 1
gt 2
le 3
```

**With the port**

`=<` is less-or-equal, as `<=`:

```
-1
-1
0
-1
0
le 1
le 2
gt 3
```

**What to change**

Nothing, if `=<` was meant as less-or-equal: the program now does what
its source says. A program that only worked because of the bug, with
`=<` where it needed "not equal", has to write `<>` there
([Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors#less-or-equal" >}})).

Issue: [The operator =< meant "not equal" (#9)](https://github.com/fubar-coder/bb2-c/issues/9).

### Numbers like `1.` and `1.0e5` {#float-literals}

Two ordinary ways of writing a float were refused by the original: a
number that ends in a decimal point, and an exponent after a fraction of
0.

**What happened**

```
a.f=1.
NPrint a
b.f=2.+1
NPrint b
End
```

The original stops at the first line with "Syntax Error".

```
a.f=1.0e5
NPrint a
b.f=2.00E3
NPrint b
End
```

The original stops at the first line with "Garbage at End of Line".

**With the port**

`1.` is the float 1 and `1.0e5` is 100000. The first program prints

```
1
3
```

and the second

```
100000
2000
```

Issues: [A number ending in a decimal point was a syntax error (#10)](https://github.com/fubar-coder/bb2-c/issues/10),
[An exponent after a fraction of 0 was ignored (#11)](https://github.com/fubar-coder/bb2-c/issues/11).

## Arrays and NewTypes

### Arrays with 256-byte elements {#arrays-256}

An array whose elements are exactly 256 bytes long, such as an array of a
NewType of 256 bytes, was indexed with the wrong element size: the
original multiplied the index by 65536 instead of 256. The same happened
with a List array of a 248-byte NewType, since a list item adds 8 bytes,
and with a field array `x.t[n]` of 256-byte elements.

**What happened**

```
NEWTYPE .rec
  v.l
  p.b[252]
End NEWTYPE
Dim a.rec(2)
NPrint SizeOf .rec
NPrint &a(1)-&a(0)
NPrint &a(2)-&a(0)
End
```

The program prints the size of an element and the distance of elements 1
and 2 from element 0. The original prints

```
256
65536
131072
```

Element 1 lies 64 KB behind element 0, far outside the array, so reading
or writing it uses memory that belongs to something else.

**With the port**

```
256
256
512
```

Issue: [Arrays with 256-byte elements had their elements 65536 bytes apart (#15)](https://github.com/fubar-coder/bb2-c/issues/15).

### A List array indexed by another array {#list-array-index}

A List array has 8 more bytes per element than a normal array of the same
type, for the list's links. When the index of one kind of array was an
element of the other kind, as in `a(b(1))`, the original used the element
size of the wrong array.

**What happened**

```
Dim List a.l(10)
Dim b.l(5)
b(1)=3
For i.l=0 To 10
  a(i)=i*100
Next
NPrint a(3)
NPrint a(b(1))
End
```

`b(1)` is 3, so both lines should print element 3 of `a`. The original
prints

```
300
100
```

**With the port**

Each array uses its own element size:

```
300
300
```

Issue: [A List array indexed by an element of another array used a wrong element size (#937)](https://github.com/fubar-coder/bb2-c/issues/937).

### A field of a NewType defined later {#newtype-defined-later}

A NewType may have a field of a type that is defined further down in the
source, once a pointer field has named that type. The original gave such
a field a wrong size, so the fields behind it and the size of the whole
NewType were wrong, also after the type was defined.

**What happened**

```
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
  y.l
End NEWTYPE
NEWTYPE .fwd
  a.l
  b.l
End NEWTYPE
DEFTYPE .dd v
v\x\b=7
v\y=9
o.l=SizeOf .dd\y
NPrint "y at ",o
NPrint "size ",SizeOf .dd
NPrint v\x\b," ",v\y
End
```

`.fwd` has 8 bytes, so `y` belongs at offset 8 and `.dd` has 12 bytes. The
original prints

```
y at 10
size 14
7 9
```

**With the port**

The NewType is laid out again when the type is defined:

```
y at 8
size 12
7 9
```

**What to change**

A program that worked with the wrong layout, for example one that reads
files written with it, has to account for the new offsets. Some sources
that the original compiled are refused now: a use of the NewType's size
before the type is defined, and two types that would contain each other
([Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors#newtype-defined-later" >}})).

Issue: [A NewType field of a type defined later got a wrong size (#974)](https://github.com/fubar-coder/bb2-c/issues/974).

## Procedures

### `End Statement` could close a Function {#end-of-procedure}

A Statement ends with `End Statement` and is left early with
`Statement Return`; a Function ends with `End Function` and is left with
`Function Return`. The original did not check that the keyword fits the
procedure it is in, so a Function closed with `End Statement`, an easy
slip when a Statement is turned into a Function, compiled without an
error.

**What happened**

```
Function$ f{}
  NPrint "in f"
End Statement
NPrint f{}
End
```

The original compiles this program. When it runs, it prints

```
in f
```

and then crashes as `f` returns, instead of printing its result: a string
Function needs the end that `End Function` gives it. Leaving a string
Function with `Statement Return` crashes the same way.

**With the port**

The compiler refuses the source with the error "Illegal End Procedure" at
the line `End Statement`. Each of the other mix-ups is an error as well:
`End Function` inside a Statement is "Illegal End Procedure", and
`Statement Return` inside a Function or `Function Return` inside a
Statement is "Illegal Procedure return".

**What to change**

Close and leave each procedure with the keywords of its own kind:

```
Function$ f{}
  NPrint "in f"
  Function Return "x"
End Function
```

A source that the original compiled and the port refuses for this reason
is described with the others in
[Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors#end-of-procedure" >}}).

Issue: [End Statement could close a Function, and the other way round (#940)](https://github.com/fubar-coder/bb2-c/issues/940).

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
start of its table, from Shape 1024 or from BitMap 512 on, the result
turned negative: `Addr`, `Use`, `Free` and the object number of a command
such as `BitMap 512,…` then addressed memory before the table instead of
the object. Runtime error checking did not notice it, since the number
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

**What to change**

Nothing. Two related bugs are not fixed yet, so avoid these commands
with such a maximum for now. `LoadShapes`, `DecodeShapes` and
`LoadSprites`, which load a whole range of objects up to the maximum,
still compute the last object with a 16-bit number and load only the
first objects ([#999](https://github.com/fubar-coder/bb2-c/issues/999)).
`Used` gives a wrong number for an object that lies 64 KB or more behind
the start of its table, such as BitMap 1025
([#998](https://github.com/fubar-coder/bb2-c/issues/998)).

Issue: [Objects past 32 KB of their table were addressed before the table (#939)](https://github.com/fubar-coder/bb2-c/issues/939).

## Include files and paths

### Text after `IncDir "…"` {#text-after-path}

The original took the whole rest of a line as the path of `IncDir`, and as
the file name of `Include`, `XInclude` and `IncBin`, including the closing
quote and anything behind it. A statement after a `:` on the same line was
never compiled.

**What happened**

```
a=1
IncDir "RAM:" : a=2
NPrint a
End
```

The original prints `1`: the `: a=2` became part of the path, and the
assignment was never compiled.

**With the port**

A path or name in quotes ends at its closing quote, and a `:` after
`IncDir` and `IncBin` starts the next statement as usual. The program
prints

```
2
```

**What to change**

After `Include` and `XInclude`, only a comment may follow the closing
quote; a statement behind a `:` there is a "Syntax Error" and needs a line
of its own
([Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors#text-after-path" >}})).

Issue: [Text after an IncDir path became part of the path (#17)](https://github.com/fubar-coder/bb2-c/issues/17).

### `IncDir` stayed for the next compile {#incdir-next-compile}

The path set by `IncDir` was never cleared. In one session of the editor,
every source compiled after a source with `IncDir`, also after a resident
made from one, looked for its include files in that directory.

**What happened**

First a resident is made from a source with the line

```
IncDir "CORPUS:nowhere/"
```

Then this program, which has an `inc.bb2` next to it, is compiled in the
same session:

```
Include "inc.bb2"
NPrint #k
End
```

The original looks for `CORPUS:nowhere/inc.bb2` and stops with "Can't open
Include" at the first line.

**With the port**

Every compile starts without an `IncDir` path. The program finds its
include file and prints the constant `#k` defined there:

```
7
```

Issue: [The path of IncDir stayed for the following compiles (#342)](https://github.com/fubar-coder/bb2-c/issues/342).

### An include name with a device after `IncDir` {#include-with-device}

After an `IncDir`, the original put its path in front of every file name,
also of a name that already starts with a device or an assign. So after
`IncDir "BlitzLibs:"` the line `Include "blitzlibs:libnums.bb"` opened
`BlitzLibs:blitzlibs:libnums.bb`.

**What happened**

```
IncDir "CORPUS:sub/"
Include "CORPUS:src/inc.bb2"
NPrint #k
End
```

The original opens `CORPUS:sub/CORPUS:src/inc.bb2` and stops with "Can't
open Include" at the second line.

**With the port**

A name with a device or assign is used as written, for `Include`,
`XInclude` and `IncBin`. The program prints

```
7
```

AB3 mode still puts the `IncDir` path in front of every name.

Issue: [After an IncDir, a file name with a device or assign could not be included (#341)](https://github.com/fubar-coder/bb2-c/issues/341).

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
of the source. When that statement was a `Dc.b` or an `IncBin` of odd
length, the original placed this code at an odd address, where the 68000
cannot run code.

**What happened**

```
NPrint "before"
End
Dc.b "abc"
```

The original prints

```
before
```

and then crashes at `End`, with a "Software Failure" requester.

**With the port**

The end code starts at an even address. The program prints

```
before
```

and ends normally.

Issue: [A program ending with an IncBin of odd length crashed at its end (#979)](https://github.com/fubar-coder/bb2-c/issues/979).

## Other fixes

The fixes below need rarer conditions. Those that make the port refuse a
source the original accepted are shown with examples on
[Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors" >}}).

### Expressions and statements

* A string addition nested seven levels deep, or passed as an argument
  of a library command from the seventh on that goes in a register, such
  as the text of `AddBox`, got a wrong length; the length is right
  ([#971](https://github.com/fubar-coder/bb2-c/issues/971)).
* A `~` inside a string literal started a constant expression, so
  `"a~1+2~b"` became `a3b`; inside strings `~` is an ordinary character
  ([#966](https://github.com/fubar-coder/bb2-c/issues/966)).
* `For a$=… To …` with a global string variable compiled and treated the
  string as a number; it gives "Bad Type for For...Next", as with a local
  string variable ([#39](https://github.com/fubar-coder/bb2-c/issues/39)).
* `Dcb` with a count of 0 made 2^32 copies and ran for hours; a count of 0
  emits nothing, a negative count gives "Illegal Constant"
  ([#37](https://github.com/fubar-coder/bb2-c/issues/37)).

### Names, labels and constants

* A constant defined without a value, a line `#c` alone, held a random
  value; its value is 0
  ([#972](https://github.com/fubar-coder/bb2-c/issues/972)).
* A constant definition without a name, `#=5`, was accepted and could make
  the compiler search 65536 bytes of memory; it gives "Syntax Error"
  ([#7](https://github.com/fubar-coder/bb2-c/issues/7)).
* A `#`, `Goto`, `Gosub`, `Restore`, `?` or `.` without a name made the
  compiler compare 65536 bytes of memory for each search; it gives "Syntax
  Error" ([#30](https://github.com/fubar-coder/bb2-c/issues/30)).
* A name of more than 127 characters, possible through a macro, overwrote
  the compiler's data; it gives "Syntax Error"
  ([#29](https://github.com/fubar-coder/bb2-c/issues/29)).
* A jump to a label with the name of a constant of value 0 gave a nonsense
  error at a line that does not exist, or read and wrote random memory; it
  gives "Label has been used as a Constant", as for any other value
  ([#977](https://github.com/fubar-coder/bb2-c/issues/977)).

### NewTypes

* A NewType that contained a field of its own type got a random size; the
  compiler reports "Illegal Type", and a pointer to the own type stays
  allowed ([#973](https://github.com/fubar-coder/bb2-c/issues/973)).

### Paths and include files

* An `IncDir` path of 96 characters or more overwrote the compiler's data,
  and the program crashed when it ran; it gives "Path too Long"
  ([#16](https://github.com/fubar-coder/bb2-c/issues/16)).
* An include file name of more than 127 characters together with the
  `IncDir` path overwrote the compiler's data; it gives "Path too Long"
  ([#325](https://github.com/fubar-coder/bb2-c/issues/325)).
* A `UsePath` path of 128 characters or more overwrote the compiler's own
  code; it gives "Path too Long"
  ([#31](https://github.com/fubar-coder/bb2-c/issues/31)).
* `IncBin` kept a file or directory in use after an error; it is closed
  ([#38](https://github.com/fubar-coder/bb2-c/issues/38)).

### Macros

* The Macro Buffer option had no effect, and blitz2 could crash when it
  ended if a source set another size; the option sets the largest macro
  text ([#336](https://github.com/fubar-coder/bb2-c/issues/336)).
* A macro line ending in `End` cut off the rest of the macro and of the
  line that called it; the rest is kept, as in AmiBlitz3
  ([#968](https://github.com/fubar-coder/bb2-c/issues/968)).
* A one-line macro with a colon right after its name, `Macro a:1:End
  Macro`, expanded to a text starting with a colon; `!a` expands to `1`
  ([#942](https://github.com/fubar-coder/bb2-c/issues/942)).
* An error inside a macro definition could crash the compiler; the error
  is shown ([#965](https://github.com/fubar-coder/bb2-c/issues/965)).
* A macro text of 65536 bytes or more was cut and part of its memory lost
  until a reboot; it gives "Macro too Big"
  ([#964](https://github.com/fubar-coder/bb2-c/issues/964)).

### Residents

* The last constant of a resident pointed behind the resident, so every
  compile freed memory that did not belong to it; the list ends where it
  should ([#34](https://github.com/fubar-coder/bb2-c/issues/34)).
* Resident files contained bytes nobody had set, so CREATE RESIDENT gave a
  different file each time; those bytes are zero
  ([#970](https://github.com/fubar-coder/bb2-c/issues/970)).
* A resident with a `NewType` without fields lost the types, macros and
  constants behind it when it was loaded; everything is loaded as defined
  ([#35](https://github.com/fubar-coder/bb2-c/issues/35)).
* A resident with a macro text of 32767 bytes or more loaded the
  definitions behind it from the wrong place; it is written and loaded
  correctly ([#995](https://github.com/fubar-coder/bb2-c/issues/995)).
* Two residents that defined the same constant with different values
  were never noticed, and the first resident's value was used silently;
  the compiler gives the warning "Clash in Residents" with the name of the
  constant and still uses the first value
  ([Warnings]({{< relref "/docs/how-blitz2-works/editor-and-compiler#warnings" >}}),
  [#36](https://github.com/fubar-coder/bb2-c/issues/36)).

### The executable

* Calls of AmigaOS functions with an argument in register a4 or a5 crashed
  or destroyed the program's variables; the compiler saves both registers
  around such a call ([#958](https://github.com/fubar-coder/bb2-c/issues/958)).
* Very large executables, with very many relocations, did not load on
  AmigaOS 2.0 and later; their relocation table is split into groups
  AmigaOS accepts ([#943](https://github.com/fubar-coder/bb2-c/issues/943)).
* Pad bytes, from `Even`, from the alignment of data and code and from
  `Ds`, held random values; they are zero
  ([#969](https://github.com/fubar-coder/bb2-c/issues/969)).
* On a 68010 processor, runtime errors and the debugger took the trap
  vectors from the wrong table when the system had moved it; the moved
  table is used ([#42](https://github.com/fubar-coder/bb2-c/issues/42)).

### Inline assembler

* After a line with a calculation on a character constant, such as
  `MOVE.w #">"*8,d0`, a `#" "` on the lines before it could become 0;
  character constants are read correctly on every line
  ([#343](https://github.com/fubar-coder/bb2-c/issues/343)).
* An inline assembler expression of 240 characters or more in a macro or
  include was freed with a wrong size; it is freed correctly
  ([#1](https://github.com/fubar-coder/bb2-c/issues/1)).
* An inline assembler operand of more than 125 characters in a macro or
  include was cut by the destination operand, and longer ones overwrote
  the compiler's data; it gives "Operand too Long"
  ([#997](https://github.com/fubar-coder/bb2-c/issues/997)).

### The compiler's memory

* After the object buffer ran full during an assignment or a `Select`,
  the compiler lost a little memory; the buffer is freed in every case
  ([#14](https://github.com/fubar-coder/bb2-c/issues/14)).
* A compile error in some assignments or in `Select` lost a little
  memory; the buffer is freed at the end of every pass
  ([#994](https://github.com/fubar-coder/bb2-c/issues/994)).
