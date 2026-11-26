---
title: Compiling
weight: 2
description: Fixed bugs of Blitz Basic 2 in what the compiler makes of a source, with examples of the ones ordinary programs meet.
aliases: [/docs/compared-with-original/fixed-bugs/compiler/]
---

These bugs of the original compiler changed what a source compiled to: a
wrong result, a program that crashed, or an error message where none
belonged. The first sections show the ones an ordinary program can run
into, each with a short program. The other fixes follow as a list, one
sentence each with a link to its issue.
The table on [Important Fixes & Breaking Changes]({{< relref "/docs/compared-with-original/important-fixes" >}})
lists all fixed bugs by what they can change in a program.

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
([Operators and expressions]({{< relref "/docs/compared-with-original/new-errors/expressions#less-or-equal" >}})).

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

### A byte compared with 128 to 255 {#byte-compare}

A byte variable holds -128 to 127. Assigning `$90` (144) stores the byte
$90, which reads back as -112. The original then compared the variable
with the number 144 itself, so the comparison never matched.

**What happened**

```
a.b=$90
If a=$90 Then NPrint "equal" Else NPrint "not equal"
End
```

The original prints `not equal`. Only `If a=-112` matched.

**With the port**

In `=` and `<>` with a byte, a constant from 128 to 255 stands for the
byte it gives, as in the assignment. The program prints `equal`, and
`If a=-112` still matches. The order comparisons stay as they were: `a<200`
is true for every byte.

Issue: [A byte variable compared with a constant from 128 to 255 never matches (#978)](https://github.com/fubar-coder/bb2-c/issues/978).

### A comparison with a Quick, or of large Longs {#quick-compare}

The compiler evaluates an expression in the type it is wanted in: the
type of the destination or parameter, or the type of the left operand
so far. The original passed that type on to both operands of a
comparison. A Quick (a number with a fraction, from -32768 to
32767.99998) wanted as a Long lost its fraction before it was compared,
so `q>1` was false for `q=1.5` when the result went to a Long. A Long
compared with a Quick was converted to a Quick first and kept only its
low 16 bits, and so were two Longs in a Quick context. In a Float
context, such as a Float destination, two Longs were compared as Float
values, which hold only about seven digits, so Longs above 16777216 that
differ slightly compared as equal.

**What happened**

```blitzbasic
q.q=1.5 : l.l=1
r.l=(q>1) : NPrint r
r.l=(q=1) : NPrint r
If q>l Then NPrint "q>l"
If l<q Then NPrint "l<q"
l2.l=100000001 : l3.l=100000000
rf.f=(l2=l3) : NPrint rf
End
```

The original prints

```
0
-1
q>l
-1
```

so it finds 1.5 equal to 1, `l<q` false although `q>l` is true, and
100000001 equal to 100000000.

**With the port**

The operands of a comparison are compared with their own values, and
only the result, -1 for true or 0 for false, takes the type of the
destination. A Long against a Quick is compared exactly, without a
conversion. The program prints

```
-1
0
q>l
l<q
0
```

Comparisons with a Float operand, of two Quicks, of whole numbers in a
whole-number context and of strings give the same results as in the original.

The same holds for a left operand that is itself a calculation. The
original computed it in the type of the destination before it read the
comparison operator, so with `q=1.25`, `r.l=(q*2>2.4)` cut `q` to 1 and
gave 0. In the port each side of a comparison is computed by its own
operands, and the result is -1. A large Long constant on the left in a
Float context, as in `rf.f=(100000001>l3)`, is compared as a Long. More
examples, also for a Word sum and a division on the left, are in
[The type of each operation]({{< relref "/docs/compared-with-original/expression-types#comparisons" >}}).

Issues: [A comparison with a Quick operand is done in Long when the context or the left operand is Long (#1177)](https://github.com/fubar-coder/bb2-c/issues/1177),
[A comparison with a compound left operand is still done in the type of the context: a Quick inside it is cut (#1183)](https://github.com/fubar-coder/bb2-c/issues/1183).

### A Quick in a Long computation {#quick-long-arithmetic}

When a computation is done in Long, because the destination or a
parameter is a Long or a Long operand stands to the left, a Quick that
comes in keeps the computation a Long one. The original cut the Quick to
a whole number at once (rounded down, so 1.5 became 1 and -1.5 became
-2) and only then did the operation, so the fraction was lost before the
arithmetic: `r.l=q*2` gave 2 for `q=1.5`, and a division by a Quick
below 1 could become a division by 0.

**What happened**

```blitzbasic
q.q=1.5 : l.l=10
r.l=q*2 : NPrint r
r.l=l-q : NPrint r
r.l=l/q : NPrint r
r.l=-q*2 : NPrint r
NPrint q*100000
End
```

The original prints

```
2
9
10
-2
100000
```

**With the port**

The operation is computed with the fraction, and its result is a whole
number: rounded down for `+`, `-`, `*` and a minus sign, towards zero
for `/`. The program prints

```
3
8
6
-3
150000
```

A Quick without a fraction gives the same results as in the original.
Shifts and the logic operators (`AND`, `OR`, `EOR`, `Not`) still take the
Quick rounded down. A program that relies on the old result writes
`Int(q)`: `r.l=Int(q)*2` gives 2. A whole chain, such as `r.l=l+q+p`, is
computed with every fraction and rounded once, when it is stored; the
examples are in
[The type of each operation]({{< relref "/docs/compared-with-original/expression-types#chains" >}}).

Issue: [Arithmetic with a Quick operand is done in Long when the context or the left operand is Long: the fraction is cut before the operation (#1178)](https://github.com/fubar-coder/bb2-c/issues/1178).

### The order of the operands {#operand-order}

The original computed an expression from left to right with one type, the
type so far, so the type of an operation depended on where its operands
stood and on what came before them. The same sum in another order could
give another result: with `q=1.5`, `NPrint 1+1+1+q` printed 4 and
`NPrint q+1+1+1` 4.5. With `l.l=10` and `p=2.75`, `r.l=l+q+p` gave 13,
because each Quick was cut before it was added. A `Mod` with a fraction
was computed on the whole parts: `x.q=q Mod 2` was 1 for `q=7.5`.

The port decides every operation by its own two operands and the type
the result is wanted in. Both orders give 4.5, a chain with a Quick under
a Long is computed exactly and rounded once (`r.l=l+q+p` is 14), and
`Mod` with a fraction is exact (1.5). Bit operators take the whole part of
a constant with a fraction, rounded down (`vb AND -1.5` is `vb AND -2`),
and a constant with a point as a shift count, such as `vb LSL 2.0`, is
the compile error "Illegal Operator for Type"
([A constant with a point as a shift count]({{< relref "/docs/compared-with-original/new-errors/expressions#shift-count-with-point" >}})).
The examples, with the original's results, the port's and the reasons,
are in
[The type of each operation]({{< relref "/docs/compared-with-original/expression-types" >}}).

Issue: [The type of an operation depends on the order of its operands and its place in a chain (#1216)](https://github.com/fubar-coder/bb2-c/issues/1216).

### `NOT`, `AND`, `OR` and `EOR` on a Quick {#quick-bit-operators}

A variable without a type suffix is a Quick, a number with 16 bits for
the whole part and 16 bits for the fraction. When an expression is
computed as a Quick, because a Quick or untyped variable is an operand or
the destination is a Quick, the original applied `NOT`, `AND`, `OR` and
`EOR` (also written `&` and `|`) to all 32 bits, the fraction included.
`NOT` turned the fraction 0 of a whole number into almost 1, so the
result was nearly 1 too high: the usual way to toggle a flag,
`flag=NOT flag`, left a flag that was true (-1) still true. `AND`, `OR`
and `EOR` combined the fractions of both operands.

**What happened**

```blitzbasic
h=True : h=NOT h : NPrint h
x=3 : q.q=NOT x : NPrint q
q=1.5 : p.q=2.5 : NPrint q OR p
End
```

The original prints

```
.99998
-3.00002
3.5
```

**With the port**

A Quick operand of these operators is first made a whole number by
dropping its fraction, which rounds it down (1.5 becomes 1, -1.5 becomes
-2), and the result is a Quick again. The program prints

```
0
-4
3
```

`h=-1`, `h=True` and `h=(a=b)` toggle to 0 and back to -1, as with a
Word flag. Whole numbers give the same results with `AND`, `OR` and
`EOR` as in the original, so masks such as `x AND 6` are unchanged. The
shifts (`LSL`, `LSR`, `ASL`, `ASR`) still work on the whole 32 bits and
keep the fraction: `x LSR 1` with `x=5` is 2.5. AB3 mode has the same
fix; AmiBlitz3 itself keeps the original's results.

**What to check in your sources**

A flag toggled with `NOT` works in the port also when it is untyped and
was set to `True`, -1 or a comparison; a program that worked around the
bug, for example with `flag=On`, works as before. Look at `AND`, `OR`
and `EOR` on values that can have a fraction: in the port a condition
such as `If h OR g` is false for `h=0.5` and `g=0`, because 0.5 counts
as 0, and true for `h=-0.5`, which counts as -1. Where the fraction matters, compare instead:
`If h<>0 OR g<>0`. A constant with a fraction is taken the same way, in
every type
([Bit operators with a fraction]({{< relref "/docs/compared-with-original/expression-types#bit-operators" >}})).

Issue: [NOT, AND, OR and EOR under a Quick type so far work on the 16.16 bits (#1209)](https://github.com/fubar-coder/bb2-c/issues/1209).

### A Float value as a condition {#float-condition}

`If`, `While` and `Until` accept any value as their condition: it is
true when the value is not 0. For a Float condition, such as `If f`,
`If 0.5`, `If f*g` or `If Abs(f)`, the original called the
floating-point test routine without handing it the value. The routine
tested whatever an earlier statement had left in a processor register
instead, so the branch did not depend on the condition at all. With one
operator, as in `If f-f`, it tested the right operand.

**What happened**

```blitzbasic
a.l=5 : b.l=0 : c.l=7
f.f=1.5 : z.f=0
x.l=a+b : If f Then NPrint "f true" Else NPrint "f false"
x=a+c : If z Then NPrint "z true" Else NPrint "z false"
End
```

Here `x=a+b` leaves 0 behind and `x=a+c` leaves 7, so the original
prints

```
f false
z true
```

A `While z` with `z=0` could run without end, a `While f` not run
at all. After a command of a library, the result depended on what that
command left behind: after an `NPrint` of a string, every Float
condition was true with the libraries of the Blitz Basic 2 CD and false
with the port's libraries.

**With the port**

A Float condition tests its own value, like a condition of any other
type. The program prints

```
f true
z false
```

Comparisons such as `If f<>0` and conditions on a Quick or a whole
number were right in the original and are unchanged, so `If f<>0` is
the form to use for a program that must also run with the original.
AB3 mode has the same fix; AmiBlitz3 itself keeps the bug.

Issue: [`If`, `While` and `Until` on a Float value test the register d1 instead of the value (#1187)](https://github.com/fubar-coder/bb2-c/issues/1187).

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
([Names, procedures and NewTypes]({{< relref "/docs/compared-with-original/new-errors/names-and-types#newtype-defined-later" >}})).

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
[Names, procedures and NewTypes]({{< relref "/docs/compared-with-original/new-errors/names-and-types#end-of-procedure" >}}).

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
([Shapes and sprites]({{< relref "/docs/compared-with-original/libraries/commands/shapeslib#shapes-and-sprites" >}})).

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
([Include files, macros and inline assembler]({{< relref "/docs/compared-with-original/new-errors/files-macros-assembler#text-after-path" >}})).

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
of the source. When that statement left an odd number of bytes - a
`Dc.b`, `Dcb.b`, `Ds.b` or `IncBin` of odd length - the original placed
this code at an odd address, where no 68000-family CPU can run code.

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

Issue: [A program ending with data of odd length crashed at its end (#979)](https://github.com/fubar-coder/bb2-c/issues/979).

## Other fixes

The fixes below need rarer conditions. Those that make the port refuse a
source the original accepted are shown with examples on
[Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors" >}}).
Each point says first what the original did, then what the port does.

### Expressions and statements

* A string addition nested seven levels deep, or passed as the seventh
  or a later register argument of a library command, such as the text of
  `AddBox`, got a wrong length; the length is right
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
* A division or `Mod` by a constant 0, such as `a.w=5/0`, crashed the
  compiler with "Software Failure" with Byte, Word or Float division and
  with Word `Mod`. With Long or Quick division, and in constants such as
  `#c=5/0`, it compiled to the largest Long number. In the port, both are
  the compile error "Division by Zero"
  ([A division by a constant 0]({{< relref "/docs/compared-with-original/new-errors/expressions#division-by-zero" >}}),
  [#1044](https://github.com/fubar-coder/bb2-c/issues/1044)).
* An operator whose right operand needed a ninth register, such as
  `a+b*c` as the 7th or a later argument of a statement or function, was
  compiled for a register that does not exist: a wrong value, a crash of
  the program with Word operands, or a crash of the compiler with
  constants. In the port, it is the compile error "Expression too
  Complex"; write `b*c+a` or `a+(b*c)` instead
  ([An operator too deep in an expression]({{< relref "/docs/compared-with-original/new-errors/expressions#expression-too-complex" >}}),
  [#1185](https://github.com/fubar-coder/bb2-c/issues/1185)).
* A Latin-1 character other than the umlauts, such as ß or é, could not
  reach a string, since the editor lost it. The port's editor keeps it as
  a literal character
  ([ß, é and other accented letters were lost]({{< relref "/docs/compared-with-original/editor#latin1-in-strings" >}})).
  Such a character gives its byte in a string, a `Data$` or `Dc.b` string
  and a quoted constant; outside a string it is a "Syntax Error" ("Token
  Not Found" in AB3 mode). In AB3 mode, the Latin-1 characters in the
  strings and comments of an AmiBlitz3 text source give the bytes
  AmiBlitz3 gives them
  ([#1018](https://github.com/fubar-coder/bb2-c/issues/1018)).
* `Supervisor_`, the first AmigaOS function of exec, gave "Syntax
  Error": the compiler looked it up in the dos library, whose token
  numbers ran into exec's; it compiles
  ([#960](https://github.com/fubar-coder/bb2-c/issues/960)).
* A Quick number just below 32768, such as `q=32767.9999`, was rounded to
  32768 and became -32768, and `Data.q` refused such a value with "Bad
  Data"; it is the largest Quick number, 32767.99998, also in `Data.q`
  ([#1070](https://github.com/fubar-coder/bb2-c/issues/1070)).
* `Data.q -32768`, the smallest Quick number, stopped the compile with
  "Bad Data", although `q=-32768` worked; it stores -32768, also when
  written `Data.q -$8000`. `Data.q 32768` stays "Bad Data"
  ([#1074](https://github.com/fubar-coder/bb2-c/issues/1074)).
* An array index after the first one was computed as a Word without
  runtime error checking and as a Long with it, so the two builds of a
  program could use different elements: with `y=200`, `g(1,y*200/2)` was
  element 52768 without the checking and 20000 with it, and with
  `q=1.5`, `b(1,q ASL 4)` was element 24 against 16. Every index is
  computed as a Long in both builds, and simple indexes such as `a(i,j)`
  or `a(5,7)` compile to the same code as in the original
  ([Array indexes]({{< relref "/docs/how-blitz2-works/compiling#array-indexes" >}}),
  [#1179](https://github.com/fubar-coder/bb2-c/issues/1179)). `AND`,
  `OR` and `EOR` with a constant with a fraction, such as
  `ja(1,vq AND 1.5)`, compile in both builds and take its whole part; a
  constant with a point as a shift count is a compile error in every
  index
  ([A constant with a point as a shift count]({{< relref "/docs/compared-with-original/new-errors/expressions#shift-count-with-point" >}}),
  [The type of each operation]({{< relref "/docs/compared-with-original/expression-types#bit-operators" >}})).
* A comparison as the right operand of an operator, such as
  `lw AND (a=b)` or `lw AND a=b`, gives a Word (-1 or 0), and the original
  converted the operator's left operand down to Word before the
  operation: a Long lost its high word, a Quick or Float its fraction.
  `If $10000 AND (a=b)` was false although `a=b` was true; with
  `lb.l=100000`, `lb+(a=b)` gave -31073, with `q.q=1.5`, `q+(a=b)` gave 0,
  and with `fh.f=0.5`, `fh+(a=b)` gave -1. The port converts the
  comparison's result to the left operand's type and computes the
  operation in that type: the condition is true, and the results are
  99999, 0.5 and -0.5. A Byte or Word left operand gives the same result
  as in the original. `AND`, `OR` and `EOR` take only the whole part of a
  Quick ([`NOT`, `AND`, `OR` and `EOR` on a Quick](#quick-bit-operators)),
  so `If q OR (a<>b)` is false for `q=0.5`, as in the original. A Float
  has no `AND`, `OR`, `EOR` or shift, so a Float left of one of them with
  a comparison on the right is a compile error
  ([A Float with `AND`, `OR`, `EOR` or a shift and a comparison]({{< relref "/docs/compared-with-original/new-errors/expressions#float-bit-operator-comparison" >}}),
  [#1182](https://github.com/fubar-coder/bb2-c/issues/1182)).
* When the type so far was Byte (a Byte destination, parameter, field or
  array element), the original cut a constant with a fraction to a whole
  number before the operation, where under a Word it stays a Float: with
  `vb.b=2`, `rb.b=(1.5*vb)` gave 2 and `rb.b=(1.5+1.5)` 2, where a Word
  destination gives 3 and 3. One source on the Blitz Basic 2 CD has
  `cprotect.b=swordhit*.8`, which computed `swordhit*0`. The port keeps
  the fraction, computes the operation in Float and converts the result
  to the Byte at the end, rounded towards zero: both give 3, as under a
  Word. A whole-valued constant written with a point, such as
  `rb.b=2.0`, and a value of `Data.b` (`Data.b 1.5` is 1) give the
  original's results. `AND`, `OR`, `EOR` and `NOT` take the whole part of
  such a constant, rounded down: `rb.b=1.5 AND vb` is `1 AND vb`, as in the
  original, but `rb.b=NOT -1.5` is 1 where the original, which cut -1.5 to
  -1, gave 0. A constant with a point as a shift count is a compile error
  ([A constant with a point as a shift count]({{< relref "/docs/compared-with-original/new-errors/expressions#shift-count-with-point" >}}),
  [The type of each operation]({{< relref "/docs/compared-with-original/expression-types#bit-operators" >}}),
  [#1188](https://github.com/fubar-coder/bb2-c/issues/1188)).
* `AND`, `OR` and `EOR` of two Byte values with different signs left a
  wrong upper byte behind, so the result was wrong as soon as it was
  used as a Word or Long: with `mb.b=-2` and `zb.b=0`,
  `If (mb AND zb)=zw` was false for `zw.w=0`, because `mb AND zb` came out
  as -256 instead of 0. The result is now the same as in a Word
  calculation ([#1218](https://github.com/fubar-coder/bb2-c/issues/1218)).
* An AmigaOS function (`Name_`) after a Float calculation in the same
  expression made the Float operations after it go through that
  library: `Abs(-2)+FindTask_(0)` crashed and `x*y+IEEESPFix_(i)` gave a
  wrong value. The Float operations use their own library again; an
  AmigaOS function before the Float part was not affected
  ([#1211](https://github.com/fubar-coder/bb2-c/issues/1211)).
* In an assignment to an array element whose index calls a library
  function such as `Abs`, the original assumed on the right side that
  this function's library was already loaded, although the right side
  runs first: `a(Abs(-j)) = Abs(-5)` could store -5 or crash, depending
  on what ran before. The right side loads the library itself
  ([#1212](https://github.com/fubar-coder/bb2-c/issues/1212)).
* In an assignment to a string array element whose index itself works
  with strings, the index overwrote the string about to be stored:
  `s$(Val(d$)) = "ab"+c$` with `d$="2"` stored `2`, a zero byte and
  `cde` instead of `abcde`. This happened with string functions such as
  `Val`, `Mid$` or `Instr`, a string comparison, or a Function that
  works with strings in the index, also for a string field of a NewType
  array. The string is stored as it was computed
  ([#1213](https://github.com/fubar-coder/bb2-c/issues/1213)).
* In an assignment to a number array element whose index works with
  strings, a right side that builds a string, such as
  `a(Val(d$)) = Len(c$+"x")`, built it wherever the string pointer
  happened to point. The value stored was right, but after `Read t$`
  this overwrote the next `Data` item, and the next `Read` crashed. The
  right side builds its string in the string work area
  ([#1214](https://github.com/fubar-coder/bb2-c/issues/1214)).

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
* A number without a fraction in a Quick calculation, such as `q=1.0`,
  was compiled as a floating-point constant and converted while the
  program runs; it is the Quick constant itself (the same value, shorter
  code) ([#13](https://github.com/fubar-coder/bb2-c/issues/13)).
* A negative whole constant in a Quick calculation, such as `On` (-1) or
  a constant `#c=-3`, came out just above its value: `q.q=On` gave
  -0.0000153, `q=#c` -2.0000153 and `q=1+On` 0.99998. It is the exact
  Quick value: -1, -3 and 0. A minus sign before a number or constant,
  as in `q=-3`, gave the right value in the original too, and
  `flag=On : flag=NOT flag` toggles the flag as in the original
  ([#1208](https://github.com/fubar-coder/bb2-c/issues/1208)).

### NewTypes

* A NewType that contained a field of its own type got a random size; the
  compiler reports "Illegal Type", and a pointer to the own type stays
  allowed ([#973](https://github.com/fubar-coder/bb2-c/issues/973)).
* An array field of a NewType with 65536 or more elements, such as
  `a.b[65540]`, or with a negative count got the size of the count
  modulo 65536, here 4 bytes, and without runtime error checking a
  program wrote past the field. Such a field is the compile error "Type
  too Big", as is a NewType of 32768 bytes or more
  ([An array field of 65536 elements or more]({{< relref "/docs/compared-with-original/new-errors/names-and-types#newtype-field-count" >}}),
  [#1084](https://github.com/fubar-coder/bb2-c/issues/1084)).

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
* A compile error on a line that came from a macro kept the memory of the
  expanded line, in a nested macro also that of the outer one, until the
  next compile or for good; it is freed with the error
  ([#1203](https://github.com/fubar-coder/bb2-c/issues/1203)).
* A line typed in the debugger's direct mode that used a macro kept the
  memory of the expanded line until the next macro or compile, and for good
  if blitz2 ended first; it is freed once the line is compiled
  ([#1204](https://github.com/fubar-coder/bb2-c/issues/1204)).
* A macro whose last line ended in some commands or operators (`NOT`,
  `OR`, `MOD`, `Print`, `Len` and others) kept a colon at its end: a
  calling line that went on after the macro, as in `c = !m b` with the
  text `NOT`, gave "Syntax Error"; the macro text ends without the colon
  ([#1206](https://github.com/fubar-coder/bb2-c/issues/1206)).
* The blank between a macro call and the rest of its line was dropped,
  so a macro text ending in a letter or digit was glued to the next name
  or number: `!mw b` with the text `!mq` became `!mqb` ("Macro not Found"
  or another macro); the blank is kept
  ([#1207](https://github.com/fubar-coder/bb2-c/issues/1207)).

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
* When the code of a program had an odd number of words, CREATE
  EXECUTABLE filled the last word of the code with 2 bytes from behind the
  compiler's own memory. On a plain system these bytes are 0, so you would
  only have noticed it under a memory checker or a similar tool that fills
  the memory behind each allocation: the same source then gave a different
  executable from compile to compile. The program ran the same either way.
  The compiler writes this word as zero, also in AB3 mode
  ([#1184](https://github.com/fubar-coder/bb2-c/issues/1184)).
* On a 68010 processor, runtime errors and the debugger took the trap
  vectors from the wrong table when the system had moved it; the moved
  table is used ([#42](https://github.com/fubar-coder/bb2-c/issues/42)).
* `TokeJsr` in a source and `ALibJsr` in a library call the routine of a
  library command. Without runtime error checking, such a call to a
  command routine that has an error check went through an extra stub, a
  `jsr` to the routine and an `rts`. Each such call cost 8 bytes and one
  extra call, for a check that is not part of the program. In the port,
  the call goes straight to the routine
  ([#1048](https://github.com/fubar-coder/bb2-c/issues/1048)).
* An operation of two constants, such as `7 Mod 3`, is worked out while
  compiling. When it was the first use of a library, such as the one of
  `Mod`, the addresses in that library's code were left out of the list
  AmigaOS uses to adjust addresses when it loads the program. They
  pointed into the compiler's memory, so a later `Mod` of Long values
  jumped there and worked only while that memory was unchanged; the
  addresses are in the list
  ([#1049](https://github.com/fubar-coder/bb2-c/issues/1049)).

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
* `PutReg` into a local variable of a `Statement` or `Function` stored the
  register next to the global variables, overwriting 1 to 4 bytes there,
  and the local kept its old value; it is stored into the local. A program
  that reads the local afterwards now sees the register's value, for
  example GMODPLAY's `GMInitMusic` on the CD, which reports a failed init
  of a module; with the original it always reported success
  ([#1202](https://github.com/fubar-coder/bb2-c/issues/1202)).

### The progress bar while compiling

* While compiling from TED on a screen with 3641 or more bytes per row
  (wide RTG modes), the compiler's progress bar wrote about 28 KB before
  TED's screen memory. The port's compiler reports its progress to TED, which
  draws the bar itself; with the original TED, the port's compiler shows
  no bar ([#980](https://github.com/fubar-coder/bb2-c/issues/980)).

### The compiler's memory

* After the object buffer ran full during an assignment or a `Select`,
  the compiler lost a little memory; the buffer is freed in every case
  ([#14](https://github.com/fubar-coder/bb2-c/issues/14)).
* A compile error in some assignments or in `Select` lost a little
  memory; the buffer is freed at the end of every pass
  ([#994](https://github.com/fubar-coder/bb2-c/issues/994)).
* An operation of two constants, such as `3 LSL 2`, is worked out while
  compiling, by running the code just compiled for it. When that code
  ended exactly at the end of the object buffer, the compiler ran on into
  the memory behind the buffer and crashed or hung. The compiler skips
  the operation when the buffer is full and handles the full buffer as
  always: the requester "Not Enough Object Memory Available!", or with
  **Make Smallest Code** a second pass with a buffer of the right size
  ([Make Smallest Code]({{< relref "/docs/how-blitz2-works/compiling#make-smallest-code" >}}),
  [#1190](https://github.com/fubar-coder/bb2-c/issues/1190)).
