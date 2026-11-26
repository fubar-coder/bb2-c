---
title: Operators and expressions
weight: 1
description: Comparisons, constant expressions, For, Dcb, divisions by a constant 0, expressions too deep for the registers and the operands of the bit operators and shifts that Blitz Basic 2 compiled and the BB2 C port refuses or compiles differently.
---

These sources compiled with Blitz Basic 2, mostly to a wrong result or to
a crash. The BB2 C port refuses them or compiles them to what they say.
Each program can be copied; a comment
behind a line, or above it, gives what the original did and what the port
does with that line. A program that the port compiles is run by the
project's tests; one that it refuses is shown as plain text. The
original's results were recorded in an emulator, with Blitz Basic 2 built
from its published source and the libraries of the Blitz Basic 2 CD.

## `=<` as "not equal" {#less-or-equal}

The original compiled `=<` as "not equal". The port reads it as
less-or-equal, the same as `<=`.

```blitzbasic
a=1 : b=2 : NPrint a=<b    ; original: -1    port: -1
a=2 : b=2 : NPrint a=<b    ; original: 0     port: -1
a=3 : b=2 : NPrint a=<b    ; original: -1    port: 0
```

A comparison gives -1 for true and 0 for false. The original's `=<` was
true wherever the two values differ, without any message. Where a source
meant less-or-equal, it now does what it says; where it relied on "not
equal", write `<>`. The bug is shown in full on
[`=<` meant "not equal"]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#less-or-equal" >}}).

Issue: [The operator =< meant "not equal" (#9)](https://github.com/fubar-coder/bb2-c/issues/9).

## A `~` inside a string {#tilde-in-string}

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

## `For` with a string variable {#for-string}

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

## `Dcb` with a count of 0 or less {#dcb-count}

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

## A division by a constant 0 {#division-by-zero}

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
([A Long or Quick division by 0]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#long-division-by-zero" >}})).

**What to change:** write the value you meant instead of the division.

Issue: [A division or Mod by a constant 0 crashed the compiler (#1044)](https://github.com/fubar-coder/bb2-c/issues/1044).

## An operator too deep in an expression {#expression-too-complex}

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

## A Float with `AND`, `OR`, `EOR` or a shift and a comparison {#float-bit-operator-comparison}

When the right operand of an operator was a comparison, the original
converted the left operand down to a Word first, so a Float was cut to a
whole number. A Float has none of these operators, and the port says so.

```
l1.l=1 : fh.f=0.5 : rf.f=0
; original: 0    port: "Illegal Operator for Type"
rf=fh AND (l1=l1) : NPrint rf
End
```

The port converts the comparison's result to the type of the left
operand instead
([Fixed bugs: expressions and statements]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#expressions-and-statements" >}})).
A Float left of `AND`, `OR`, `EOR` or a shift is then "Illegal Operator
for Type", as it is with any other right operand.

**What to change:** write `Int(fh) AND (l1=l1)`, which gives the
original's result, or use a Long variable.

Issue: [A comparison as the right operand converts the left operand down to Word: a Long loses its high word, a Float or Quick is cut (#1182)](https://github.com/fubar-coder/bb2-c/issues/1182).

## A constant with a point as a shift count {#shift-count-with-point}

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

Issue: [The type of an operation depends on the order of its operands and its place in a chain (#1216)](https://github.com/fubar-coder/bb2-c/issues/1216).
