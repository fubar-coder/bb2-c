---
title: Expressions
weight: 3
prev: /docs/compared-with-original/language
next: /docs/compared-with-original/expressions/type-system
description: Fixed bugs of Blitz Basic 2 in the value of an expression - operators and numbers, comparisons, Quicks in Long computations, the order of the operands, conditions, constant calculations and expressions too deep for the registers.
aliases: [/docs/compared-with-original/new-errors/expressions/]
# the entries of the old page new-errors/expressions that are on another page
anchorMoves:
  tilde-in-string: language
  for-string: language
  dcb-count: language
---

These bugs of the original compiler changed the value of an expression:
an operator that meant something else, a number that was refused, a
comparison or a calculation done in the wrong type, a condition that
tested the wrong value, and calculations of constants or of deep
expressions that crashed the compiler or took a wrong value. The first
sections show the ones an ordinary program can run into, each with a
short program. The other fixes follow in tables by topic.

How the port decides the type in which each operation of an expression
is computed, with programs that give the original's and the port's
results side by side, is described on the sub-page
[Expression type system]({{< relref "/docs/compared-with-original/expressions/type-system" >}}).
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
[Expression type system]({{< relref "/docs/compared-with-original/expressions/type-system#comparisons" >}}).

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
[Expression type system]({{< relref "/docs/compared-with-original/expressions/type-system#chains" >}}).

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
[Expression type system]({{< relref "/docs/compared-with-original/expressions/type-system" >}}).

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
([Shifts]({{< relref "/docs/compared-with-original/expressions/type-system#shifts" >}})).

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
([Bit operators with a fraction]({{< relref "/docs/compared-with-original/expressions/type-system#bit-operators" >}})).

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

## Other fixes

The fixes below need rarer conditions. Each row of a table says what the
original did and what the port does, and links the issue with the
details. A fix that makes the port refuse a source the original accepted,
or that needs more room, follows the table of its group as an entry of
its own, with a program whose comments give what the original did and
what the port does, and what to change in a source. A program that the
port compiles is run by the project's tests; one that it refuses is shown
as plain text.

### Numbers and constants {#numbers}

| What was wrong | Now | Issue |
|---|---|---|
| A Quick number just below 32768, such as `q=32767.9999`, was rounded to 32768 and became -32768, and `Data.q` refused such a value with "Bad Data". | It is the largest Quick number, 32767.99998, also in `Data.q`. | [#1070](https://github.com/fubar-coder/bb2-c/issues/1070) |
| `Data.q -32768`, the smallest Quick number, stopped the compile with "Bad Data", although `q=-32768` worked. | It stores -32768, also when written `Data.q -$8000`. `Data.q 32768` stays "Bad Data". | [#1074](https://github.com/fubar-coder/bb2-c/issues/1074) |
| A number without a fraction in a Quick calculation, such as `q=1.0`, was compiled as a floating-point constant and converted while the program runs. | It is the Quick constant itself: the same value, shorter code. | [#13](https://github.com/fubar-coder/bb2-c/issues/13) |
| A negative whole constant in a Quick calculation, such as `On` (-1) or a constant `#c=-3`, came out just above its value: `q.q=On` gave -0.0000153, `q=#c` -2.0000153 and `q=1+On` 0.99998. | It is the exact Quick value: -1, -3 and 0. A minus sign before a number or constant, as in `q=-3`, gave the right value in the original too, and `flag=On : flag=NOT flag` toggles the flag as in the original. | [#1208](https://github.com/fubar-coder/bb2-c/issues/1208) |

### Operations {#expressions-and-statements}

| What was wrong | Now | Issue |
|---|---|---|
| A string addition nested seven levels deep, or passed as the seventh or a later register argument of a library command, such as the text of `AddBox`, got a wrong length. | The length is right. | [#971](https://github.com/fubar-coder/bb2-c/issues/971) |
| `AND`, `OR` and `EOR` of two Byte values with different signs left a wrong upper byte behind, so the result was wrong as soon as it was used as a Word or Long: with `mb.b=-2` and `zb.b=0`, `If (mb AND zb)=zw` was false for `zw.w=0`, because `mb AND zb` came out as -256 instead of 0. | The result is the same as in a Word calculation. | [#1218](https://github.com/fubar-coder/bb2-c/issues/1218) |

#### An array index after the first one {#later-array-index}

The original computed an array index after the first one as a Word
without runtime error checking and as a Long with it, so the two builds of
a program could use different elements: with `y=200`, `g(1,y*200/2)` was
element 52768 without the checking and 20000 with it, and with `q=1.5`,
`b(1,q ASL 4)` was element 24 against 16.

The port computes every index as a Long in both builds, and simple
indexes such as `a(i,j)` or `a(5,7)` compile to the same code as in the
original
([Array indexes]({{< relref "/docs/how-blitz2-works/compiling#array-indexes" >}})).
`AND`, `OR` and `EOR` with a constant with a fraction, such as
`ja(1,vq AND 1.5)`, compile in both builds and take its whole part; a
constant with a point as a shift count is a compile error in every index
([A constant with a point as a shift count](#shift-count-with-point),
[Bit operators with a fraction]({{< relref "/docs/compared-with-original/expressions/type-system#bit-operators" >}})).

Issue: [A later array index is evaluated as a Word without runtime error checking and as a Long with it: the two builds address different elements (#1179)](https://github.com/fubar-coder/bb2-c/issues/1179).

#### A constant with a fraction under a Byte {#byte-fraction-constant}

When the type so far was Byte (a Byte destination, parameter, field or
array element), the original cut a constant with a fraction to a whole
number before the operation, where under a Word it stays a Float: with
`vb.b=2`, `rb.b=(1.5*vb)` gave 2 and `rb.b=(1.5+1.5)` 2, where a Word
destination gives 3 and 3. One source on the Blitz Basic 2 CD has
`cprotect.b=swordhit*.8`, which computed `swordhit*0`.

The port keeps the fraction, computes the operation in Float and converts
the result to the Byte at the end, rounded towards zero: both give 3, as
under a Word. A whole-valued constant written with a point, such as
`rb.b=2.0`, and a value of `Data.b` (`Data.b 1.5` is 1) give the
original's results. `AND`, `OR`, `EOR` and `NOT` take the whole part of
such a constant, rounded down: `rb.b=1.5 AND vb` is `1 AND vb`, as in the
original, but `rb.b=NOT -1.5` is 1 where the original, which cut -1.5 to
-1, gave 0. A constant with a point as a shift count is a compile error
([A constant with a point as a shift count](#shift-count-with-point),
[Bit operators with a fraction]({{< relref "/docs/compared-with-original/expressions/type-system#bit-operators" >}})).

Issue: [Under a Byte destination a constant with a fraction is cut before the operation (#1188)](https://github.com/fubar-coder/bb2-c/issues/1188).

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

#### A constant Mod by a divisor below 1 {#mod-fraction-divisor}

When both operands of a Quick `Mod` are constants and the divisor is
smaller than 1, such as `0.5`, the original crashed the compiler with
"Software Failure": it worked out the whole parts of the two numbers, and
the whole part of the divisor is 0. The port compiles the line and gives
the exact `Mod`, the same as for variables.

```
; original: Software Failure in the compiler    port: 0
rq.q=5 Mod 0.5
; original: Software Failure in the compiler    port: 0.25
rq.q=5.25 Mod 0.5
```

A divisor of exactly 0 stays the compile error
([A division by a constant 0](#division-by-zero)).

**What to change:** nothing; the port accepts the line. With a variable
divisor below 1 the original fails as well, when the program runs
("Software Failure"); the port gives the exact `Mod` there too.

Issue: [A constant Quick Mod by a divisor below 1 crashed the compiler (#1219)](https://github.com/fubar-coder/bb2-c/issues/1219).

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
[Expression type system]({{< relref "/docs/compared-with-original/expressions/type-system#edge-cases" >}}).

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
