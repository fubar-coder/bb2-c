---
title: Compiling
weight: 2
description: Fixed bugs of Blitz Basic 2 in what the compiler makes of a source, with examples of the ones ordinary programs meet.
aliases: [/docs/compared-with-original/fixed-bugs/compiler/, /docs/compared-with-original/new-errors/expressions/, /docs/compared-with-original/new-errors/names-and-types/, /docs/compared-with-original/new-errors/files-macros-assembler/]
# an entry of the old page new-errors/names-and-types that is on another page
anchorMoves:
  latin1-outside-string: editor
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
`=<` where it needed "not equal", has to write `<>` there.

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
([A constant with a point as a shift count](#shift-count-with-point)).
The examples, with the original's results, the port's and the reasons,
are in
[The type of each operation]({{< relref "/docs/compared-with-original/expression-types" >}}).

Issue: [The type of an operation depends on the order of its operands and its place in a chain (#1216)](https://github.com/fubar-coder/bb2-c/issues/1216).

#### A constant with a point as a shift count {#shift-count-with-point}

The count of `LSL`, `LSR`, `ASL` and `ASR` is a whole number of bits. The
original accepted a constant written with a point as the count in many
places and shifted by its whole part; the port refuses it everywhere.

```
vb.b=3 : vq.q=1.5
rb.b=vb LSL 2.5    ; original: 12    port: "Illegal Operator for Type"
rb.b=vq LSL 2.5    ; original: 6     port: "Illegal Operator for Type"
rb.b=vb ASL 1.0    ; original: 6     port: "Illegal Operator for Type"
```

In the port a constant with a point is a number with a fraction wherever
it is an operand, also when its value is whole, as `1.0`. The original
accepted such a count only in some places, and in an array index after
the first one only without runtime error checking; the port makes this
one rule, in every type and every array index. A Quick variable as the
count is still its whole part, rounded down
([Shifts]({{< relref "/docs/compared-with-original/expression-types#shifts" >}})).

**What to change:** write the count as a whole number, such as `vb LSL 2`,
which gives the original's result.

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

```blitzbasic
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
files written with it, has to account for the new offsets.

Two kinds of source that the original compiled are refused. Using the
size of such a NewType, or a field behind the early field, before the
type is defined is the error "Type not yet defined":

```
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
End NEWTYPE
; original: compiles, prints 4    port: "Type not yet defined"
o.l=SizeOf .dd
NEWTYPE .fwd
  a.l
End NEWTYPE
NPrint o
End
```

Two types that would contain each other are "Illegal Type", and a field
of a type that is never defined is "Type Not Found":

```
NEWTYPE .pp
  *p.tb
End NEWTYPE
NEWTYPE .ta
  x.tb
End NEWTYPE
NEWTYPE .tb
  ; original: compiles, prints compiled    port: "Illegal Type"
  y.ta
End NEWTYPE
NPrint "compiled"
End
```

Move a use of the NewType's size behind the definition of the type, and
break a cycle of two types with a pointer field.

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

A Statement closes with `End Statement` and is left with
`Statement Return`. Two examples on the Blitz Basic 2 CD,
`DARTS_POLY.BB` and `DARTS_WIRE.BB`, end the Function `gendart` with
`End Statement` and are refused for this reason.

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

```blitzbasic
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
quote; a statement behind a `:` there is a "Syntax Error":

```
Include "inc.bb2" : a=1
NPrint a
End
; original: "Can't open Include"    port: "Syntax Error", both in line 1
```

The original failed even where `inc.bb2` exists, since the rest of the
line was part of the file name. Other text after the closing quote is a
"Syntax Error" too. A path without quotes still takes the rest of the
line. Put a statement that follows `Include` or `XInclude` on its own
line.

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

The fixes below need rarer conditions. Each point says first what the
original did, then what the port does. A fix that makes the port refuse a
source the original accepted follows the list of its group, with a
program whose comments give what the original did and what the port
does, and what to change in a source. A program that the port compiles is
run by the project's tests; one that it refuses is shown as plain text.

### Expressions and statements

* A string addition nested seven levels deep, or passed as the seventh
  or a later register argument of a library command, such as the text of
  `AddBox`, got a wrong length; the length is right
  ([#971](https://github.com/fubar-coder/bb2-c/issues/971)).
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
  ([A constant with a point as a shift count]({{< relref "/docs/compared-with-original/compiler#shift-count-with-point" >}}),
  [The type of each operation]({{< relref "/docs/compared-with-original/expression-types#bit-operators" >}})).
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
  ([A constant with a point as a shift count]({{< relref "/docs/compared-with-original/compiler#shift-count-with-point" >}}),
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

#### A `~` inside a string {#tilde-in-string}

Outside strings, `~expr~` puts the value of a constant expression into the
source. The original did this inside strings too; in the port a `~`
inside quotes is an ordinary character.

```blitzbasic
NPrint "a~1+2~b"    ; original: a3b    port: a~1+2~b
NPrint ~1+2~        ; original: 3      port: 3
NPrint "x~y"        ; original: "Illegal Constant Expression"    port: x~y
End
```

A single `~` in a string, as in the third line, was the compile error
"Illegal Constant Expression" in the original, so the original compiles
the program only without that line.

**What to change:** build a text from constants outside the quotes.

Issue: [A ~ inside a string literal started a constant expression (#966)](https://github.com/fubar-coder/bb2-c/issues/966).

#### `For` with a string variable {#for-string}

The original refused a local string variable as the variable of a `For`
loop, but accepted a global one and treated the string as a number. The
port refuses both.

```
; original: compiles, prints done    port: "Bad Type for For...Next"
For a$="a" To 10
Next
NPrint "done"
End
```

**What to change:** use a numeric variable for the loop.

Issue: [For a$=… To … with a global string variable compiled (#39)](https://github.com/fubar-coder/bb2-c/issues/39).

#### `Dcb` with a count of 0 or less {#dcb-count}

The original took the count of `Dcb` as an unsigned number, so a count of
0 or -1 made about four thousand million copies. In the port a count of 0
emits nothing and a negative count is an error.

```blitzbasic
Goto skip
; original: the compiler does not finish    port: no data
Dcb.w 0,0
.skip
NPrint "ok"
End
```

```
Goto skip
; original: the compiler does not finish    port: "Illegal Constant"
Dcb.w -1,0
.skip
NPrint "ok"
End
```

The original's compiler ran for hours, or until it had overwritten its
own memory.

**What to change:** leave out a `Dcb` with a negative count.

Issue: [Dcb with a count of 0 made 2^32 copies (#37)](https://github.com/fubar-coder/bb2-c/issues/37).

#### A division by a constant 0 {#division-by-zero}

When both operands of `/` or `Mod` are constants, the compiler computes
the result itself. The original did not look at the divisor first; the
port stops with "Division by Zero".

```
NPrint "before"
; original: compiles, prints 2147483647    port: "Division by Zero"
d.l=7/0
NPrint d
End
```

In the original a Long or Quick division by 0, also in a constant
definition such as `#c=5/0`, gave the largest Long number, and a Long
`Mod` by 0 gave the dividend. A Byte, Word or Float division by 0, and a
Word `Mod` by 0, crashed the compiler with "Software Failure". The port
gives the error for every type. It finds only a divisor that is a
constant; a division by a variable that holds 0 is a matter of the
running program
([A Long or Quick division by 0]({{< relref "/docs/compared-with-original/libraries/commands/ldivlib#long-division-by-zero" >}})).

**What to change:** write the value you meant instead of the division.

Issue: [A division or Mod by a constant 0 crashed the compiler (#1044)](https://github.com/fubar-coder/bb2-c/issues/1044).

#### An operator too deep in an expression {#expression-too-complex}

The compiler works out an expression in the processor's eight data
registers. The original did not check when an operator needed a ninth one
and took a wrong value; the port stops with "Expression too Complex".

```
a.l=1 : b.l=5 : c.l=6
; original: -1
NPrint RectsHit(20,0,10,10,0,0,b*c+a,10)
; original: 0    port: "Expression too Complex"
NPrint RectsHit(20,0,10,10,0,0,a+b*c,10)
End
```

Each level of an expression takes a register: when a tighter operator
such as `*`, or a part in brackets, has to be computed first, the value
on its left keeps its register and the rest goes into the next one. The
arguments of a statement or function from the 7th on already start in the
7th register, so `a+b*c` there needs a ninth register for `b*c`; so does
such an operator inside six levels of brackets. The original computed
`b*c` over `a` and took the second value of the addition from an address
register: the two rectangles overlap, but the second call got a wrong
width. With Word variables the compiled program crashed with "Software
Failure", and with constants, such as `1+5*6`, the compiler itself
crashed. Statements with many arguments, such as `InitCopList`, meet it
most easily; none of the programs on the Blitz Basic 2 CD contains such
an expression.

An expression that stays within the eight registers compiles as before:
a chain of one precedence (`a+b+c`), the tighter operator first
(`b*c+a`), or brackets around the right part (`a+(b*c)`). A calculation
of a Quick with a Long takes two registers in the port, so a few such
expressions that the original compiled reach the limit; they are on
[The type of each operation]({{< relref "/docs/compared-with-original/expression-types#edge-cases" >}}).

**What to change:** write the tighter operation first, `b*c+a`, or put it
in brackets, `a+(b*c)`; inside many levels of brackets, compute a part
into a variable first.

Issues:
[An operator whose right operand needs a ninth register took a wrong value (#1185)](https://github.com/fubar-coder/bb2-c/issues/1185),
and for the expressions with a Quick and a Long,
[The type of an operation depends on the order of its operands and its place in a chain (#1216)](https://github.com/fubar-coder/bb2-c/issues/1216).

#### A comparison as the right operand {#float-bit-operator-comparison}

A comparison as the right operand of an operator, such as
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
so `If q OR (a<>b)` is false for `q=0.5`, as in the original.

A Float has no `AND`, `OR`, `EOR` or shift. The original's conversion cut
a Float left of one of them to a whole number when a comparison stood on
the right; in the port, a Float there is "Illegal Operator for Type", as
it is with any other right operand.

```
l1.l=1 : fh.f=0.5 : rf.f=0
; original: 0    port: "Illegal Operator for Type"
rf=fh AND (l1=l1) : NPrint rf
End
```

**What to change:** write `Int(fh) AND (l1=l1)`, which gives the
original's result, or use a Long variable.

Issue: [A comparison as the right operand converts the left operand down to Word: a Long loses its high word, a Float or Quick is cut (#1182)](https://github.com/fubar-coder/bb2-c/issues/1182).

### Names, labels and constants

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

#### A constant definition without a name {#constant-without-name}

The original accepted `#=5`, a constant with an empty name. The port
refuses it.

```
; original: compiles, prints ok    port: "Syntax Error"
#=5
NPrint "ok"
End
```

A second such line made the original's compiler search 65536 bytes of
memory for the name.

**What to change:** give the constant a name, or remove the line.

Issue: [A constant definition without a name was accepted (#7)](https://github.com/fubar-coder/bb2-c/issues/7).

#### A `#`, `Goto` or `Gosub` without a name {#missing-name}

A `#`, `Goto`, `Gosub`, `Restore`, `?` or `.` without a name made the
original search its lists for a name of length 0 and report an error
that did not fit. The port reports the missing name.

```
a=#+1           ; original: "Constant Not Found"    port: "Syntax Error"
NPrint a
End
```

```
Goto            ; port: "Syntax Error"
Goto
NPrint "ok"
End
; original: "Label not Found"
```

The original compared up to 65536 bytes of memory with the empty name
before it gave up.

**What to change:** write the missing name.

Issue: [A #, Goto, Gosub, Restore, ? or . without a name made the compiler read far past the name (#30)](https://github.com/fubar-coder/bb2-c/issues/30).

#### A constant without a value {#constant-without-value}

A line with only a constant name, without `=` and a value, was accepted,
but the original never gave the constant a value. In the port such a
constant is 0.

```blitzbasic
#c
#d=5
#d
NPrint #c               ; original: a random value    port: 0
CNIF #c
NPrint "cnif true"      ; original: this line, in the recording
CELSE
NPrint "cnif false"     ; port: this line
CEND
NPrint #d               ; original: 5    port: 5
End
```

The original's constant held whatever was in the compiler's memory, so
`CNIF #c` could be true or false from one compile to the next. In the
recording, with the compiler's memory filled with the byte `$A5`, it
printed -1515870811 and `cnif true`. A constant that already had a value,
as `#d`, keeps it in both.

**What to change:** give the constant the value it needs, `#c=1` for
example.

Issue: [A constant defined without a value held a random value (#972)](https://github.com/fubar-coder/bb2-c/issues/972).

#### A name of more than 127 characters {#long-name}

The original copied a name into a buffer of 128 bytes without a limit,
and a longer name overwrote the compiler's data. The port accepts names
of up to 127 characters.

```
Macro s
`1`1`2=1
NPrint `1`1`2
End Macro
; a name of 127 characters    original: prints 1    port: prints 1
!s{aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,aaaaaaa}
; a name of 128 characters    original: prints 1    port: "Syntax Error"
!s{bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,bbbbbbbb}
End
```

A line of the editor has at most 128 characters, so such a name can only
come from a macro that joins its parameters, as `s` does here.

**What to change:** use names of at most 127 characters.

Issue: [A name of more than 127 characters overwrote the compiler's data (#29)](https://github.com/fubar-coder/bb2-c/issues/29).

### NewTypes

#### A NewType that contains itself {#newtype-contains-itself}

A NewType cannot contain a field of its own type: it would have to be
larger than itself. The original gave such a field a random size; the
port refuses it.

```
NEWTYPE .selft
  x.selft                       ; port: "Illegal Type"
  y.l
End NEWTYPE
DEFTYPE .selft v
v\y=5
o.l=SizeOf .selft\y
NPrint "size ",SizeOf .selft    ; original: size 262
NPrint "y at ",o                ; original: y at 258
NPrint "y ",v\y                 ; original: y 5
End
```

In the original the fields behind such a field and the size of the whole
type were random too; the values above were recorded with the compiler's
memory filled with the byte `$01`. What such a source usually means is a
pointer to another item of the same type, and a pointer field is
allowed:

```blitzbasic
NEWTYPE .node
  value.l
  *child.node
End NEWTYPE
```

Issue: [A NewType field of its own type took a random size (#973)](https://github.com/fubar-coder/bb2-c/issues/973).

#### An array field of 65536 elements or more {#newtype-field-count}

A field of a NewType can be an array of a fixed number of elements, such
as `a.b[10]`, a field of ten bytes. The original kept the count in 16
bits; the port checks the whole count.

```
NEWTYPE .t
  a.b[65540]        ; original: a field of 4 bytes    port: "Type too Big"
End NEWTYPE
NPrint SizeOf .t    ; original: 4
End
```

A NewType must stay below 32768 bytes, so a larger one is the error
"Type too Big". The original took a count of 65536 or more, and a
negative one such as `a.b[-65532]`, modulo 65536, so the NewType passed
the size check. With runtime error checking an index behind the fourth
element stopped the program; without it, a program that used those
elements wrote over the memory behind the field.

**What to change:** give the array a count that fits the NewType, or keep
the data in an array of its own with `Dim`.

Issue: [An array field's element count of 65536 or more is taken modulo 65536 without an error (#1084)](https://github.com/fubar-coder/bb2-c/issues/1084).

### Paths and include files

* `IncBin` kept a file or directory in use after an error; it is closed
  ([#38](https://github.com/fubar-coder/bb2-c/issues/38)).

#### Paths that are too long {#path-too-long}

The original copied paths into buffers of a fixed size without a limit.
The port stops with "Path too Long" at the line of the path.

```
; a path of 98 characters
; original: compiles, the program crashes    port: "Path too Long"
IncDir "ppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
x=5
NPrint x
End
```

The original's program crashed with a "Software Failure" instead of
printing 5. Three limits are checked:

* an `IncDir` path of 96 characters or more, which overwrote the
  compiler's data;
* the `IncDir` path and the name of an `Include`, `XInclude` or `IncBin`
  together, which could overwrite the compiler's data from 128
  characters on;
* a `UsePath` path of 128 characters or more, possible through a macro,
  which overwrote the compiler's own code, so that the compiler did not
  finish.

**What to change:** use a shorter path, for example with an assign.

Issues:
[IncDir path of 96 characters or more (#16)](https://github.com/fubar-coder/bb2-c/issues/16),
[include file name of more than 127 characters (#325)](https://github.com/fubar-coder/bb2-c/issues/325),
[UsePath path of 128 characters or more (#31)](https://github.com/fubar-coder/bb2-c/issues/31).

### Macros

* A one-line macro with a colon right after its name, `Macro a:1:End
  Macro`, expanded to a text starting with a colon; `!a` expands to `1`
  ([#942](https://github.com/fubar-coder/bb2-c/issues/942)).
* An error inside a macro definition could crash the compiler; the error
  is shown ([#965](https://github.com/fubar-coder/bb2-c/issues/965)).
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

#### A macro line ending in `End` {#macro-end-line}

In the original a line of a macro that ended in `End` cut off the rest of
the macro and of the line that called it. The port keeps the rest of the
macro, as AmiBlitz3 does.

```blitzbasic
Macro m
  If `1
    End
  End If
  NPrint "after"
End Macro
!m{0}            ; original: "If Without End If"    port: after
NPrint "done"    ; port: done
End
```

The original lost the `End If` and the rest of the macro, so it stopped
at the call of the macro.

**What to change:** nothing, unless a source relied on the cut; then
remove the lines that the original left out.

Issue: [A macro line ending in End cut off the rest of the macro (#968)](https://github.com/fubar-coder/bb2-c/issues/968).

#### A macro larger than the Macro Buffer {#macro-buffer}

The compiler options have a **Macro Buffer** size. In the original it had
no effect: the buffer for macro texts always had its default size of
8192 bytes. In the port the option counts, in both directions.

A source with a macro text of 9120 bytes, whose options set the Macro
Buffer to 16384 bytes, was refused by the original with "Macro too Big"
inside the macro, and blitz2 hung afterwards; the port compiles it. A
source with a macro text of 1520 bytes, whose options set the buffer to
1024 bytes, compiled in the original; the port gives "Macro too Big" at
the line where the macro's text passes the size. A source that set
another size could also make blitz2 crash when it ended. A macro text of
more than 65535 bytes is always "Macro too Big"; the original cut such a
text, and part of its memory was lost until a reboot.

The buffer gets its new size when a source is loaded or the COMPILER
OPTIONS are closed. If there is not enough memory for it, the compiler
reports out of memory and keeps the old buffer with its size; the next
change of the options tries again.

**What to change:** a source saved with the default size is not affected.
If one of yours fails with "Macro too Big", set its Macro Buffer back to
the default or larger in the COMPILER OPTIONS.

Issues:
[The Macro Buffer option had no effect (#336)](https://github.com/fubar-coder/bb2-c/issues/336),
[A macro text of 65536 bytes or more was cut (#964)](https://github.com/fubar-coder/bb2-c/issues/964).

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
* `PutReg` into a local variable of a `Statement` or `Function` stored the
  register next to the global variables, overwriting 1 to 4 bytes there,
  and the local kept its old value; it is stored into the local. A program
  that reads the local afterwards now sees the register's value, for
  example GMODPLAY's `GMInitMusic` on the CD, which reports a failed init
  of a module; with the original it always reported success
  ([#1202](https://github.com/fubar-coder/bb2-c/issues/1202)).

#### An operand that is too long {#operand-too-long}

The compiler keeps the text of an inline assembler operand in a buffer of
a fixed size when the text comes from a macro or an include, or contains a
local label. The original copied the text without a limit; the port stops
with "Operand too Long".

```
Macro m
move.l #`1+`1,dat
End Macro
Goto skip
.dat
Dc.l 0
.skip
; a source operand of 127 characters
; original: "Syntax Error"    port: "Operand too Long"
!m{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1}
NPrint Peek.l(?dat)
End
```

There is one buffer for the source operand and `Dc`, and one for the
destination operand right behind it. In the original a source operand of
more than 125 characters reached into the destination's buffer, and the
destination overwrote its end: wrong code or "Syntax Error", as here,
where the program should store 64. Longer texts overwrote the compiler's
data behind the buffers: a source operand or `Dc` of more than 261
characters, a destination operand of more than 133. The port reports
each of these at the line of the instruction, here the call of the
macro. Every text that fits compiles as before, also a source operand of
up to 261 characters when the destination is a register.

**What to change:** split the expression, for example into a constant
defined before the instruction, or use shorter macro parameters.

Issue: [copyconst copies an operand text into namebuff without a length check (#997)](https://github.com/fubar-coder/bb2-c/issues/997).

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
