---
title: The type of each operation
weight: 3
description: How Blitz Basic 2 and the BB2 C port decide the type in which each operation of an expression is computed, with the results that differ, example by example - the order of the operands, chains, a Quick beside a Long, Mod, comparisons, bit operators, shifts, Select and the edge cases.
prev: /docs/compared-with-original/fixed-bugs/editor
next: /docs/compared-with-original/new-errors
---

Before this page you should know the five number types of Blitz Basic 2
and the type so far, both explained in
[The type of an expression]({{< relref "/docs/how-blitz2-works/compiling#the-type-of-an-expression" >}}).

Every operation in an expression is computed in one of the number types:
Byte, Word, Long, Quick or Float. A Quick is the default type of a
variable without a suffix: a number with 16 bits for the whole part and 16
bits for the fraction, from -32768 to 32767.99998. Blitz Basic 2 and the
BB2 C port decide this type in different ways, and so a program that mixes
Quicks with other types can get other results with the port. This page
shows these differences, each with the original's result, the port's
result and the reason for both.

The outputs of the original are those of Blitz Basic 2 built from its
published source and of blitz2 1.95 from the Blitz Basic 2 CD, recorded in
an emulator. AB3 mode types expressions the same way as BB2 mode;
AmiBlitz3 itself keeps the original's rule.

## The two rules

**The original** reads an expression from left to right and keeps one
type, the *type so far*. It starts with the type the result is wanted in:
the type of the destination variable or of the parameter. `If`, `While`,
`Until`, `Select` and `Print` want no type, so there the first operand
sets it. Each operand raises the type so far if its own type is higher,
and is converted to it if it is lower. One exception: under a Long type so
far a Quick does not raise the type but is cut to a whole number (rounded
down) as soon as it is fetched. So the type of an operation depends on
where its operands stand in the expression and on what came before them.

**The port** first reads the whole expression and then decides every
operation by three things: the operator, the types of its two operands,
and the wanted type (the type of the destination or parameter; a Long for
an array index; none for `If`, `While`, `Until`, `Select`, `Print` and the
two sides of a comparison):

* Whole numbers are computed in their own types, as in the original.
* A Quick with a Quick, a Byte or a Word is computed as a Quick, with the
  original's routines and results.
* A Quick with a Long, and every operation with a Quick where a whole
  number is wanted, is computed exactly, with a whole part of 48 bits and
  16 bits of fraction (this page calls it "the exact format"). A Float
  operand makes the operation a Float one.
* The result is rounded once, where it is taken: to a whole number by
  rounding down (a `/` where a Long is wanted towards zero), to a Quick
  for a Quick destination, and where nothing is wanted it stays a Quick
  if it fits, else its whole part.
* A constant written with a point, such as `1.5` or `2.0`, is a number
  with a fraction wherever it is an operand: a Quick beside a Quick, else
  a Float.

In the port the order of the operands does not matter: `a+b` and `b+a` are
computed in the same type.

## The order of the operands {#order}

With `q.q=1.5 : w.w=3 : v.w=4 : l.l=10` and `NPrint`, which wants no type:

| Expression | Original | Port |
|---|---|---|
| `NPrint 1+1+1+q` | 4 | 4.5 |
| `NPrint q+1+1+1` | 4.5 | 4.5 |
| `NPrint w+v+q` | 8 | 8.5 |
| `NPrint q+w+v` | 8.5 | 8.5 |
| `NPrint w*v+q` | 13 | 13.5 |
| `NPrint q+&l-&l` | 1 | 1.5 |

In the original, the first operand sets the type so far. `1+1+1` and `w+v`
are whole numbers that end up as a Long (two Words added or multiplied
give a Long), and a Quick that comes after a Long is cut to 1, so the
fraction is lost. When the Quick comes first, the type so far is Quick
and the whole numbers are converted to it. In `q+&l-&l` the original adds
and subtracts the address and the Quick in a way that loses the fraction.

In the port, `w+v` is a whole-number sum, and adding the Quick gives a
Quick, whatever the order. With the address the operation is computed in
the exact format, and the result, 1.5, fits a Quick.

A division of whole numbers beside a Quick follows the same rule. With
`q=0.5` and `w=3`:

| Expression | Original | Port |
|---|---|---|
| `NPrint q+w/2` | 2 | 2 |
| `NPrint w/2+q` | 1.5 | 2 |
| `If w/2>q` with `q=1.2` | false | true |

In the original `w/2` is a whole-number division (1) when it comes first
and a division with a fraction (1.5) when the Quick comes first. In the
port the division beside a Quick is always computed with the fraction.

One thing still depends on the order, in both: an intermediate value that
leaves its type's range. With `q=300 : q2=300 : r=0.25`, `x.q=q*q2*r`
gives 6116, because `q*q2` (90000) wraps as a Quick, and `x.q=r*q*q2`
gives 22500.

## A chain under a Long {#chains}

With `l.l=10 : q.q=1.5 : p.q=2.75 : f.f=0.5` and a Long destination:

| Expression | Original | Port |
|---|---|---|
| `r.l=l+q+p` | 13 | 14 |
| `r.l=q+p+l` | 13 | 14 |
| `r.l=(q+p)*l` | 30 | 42 |
| `r.l=l+q+f` | 11 | 12 |
| `r.l=f+q+l` | 12 | 12 |

The destination is a Long, so in the original the type so far is Long
from the start, and each Quick is cut to a whole number when it is
fetched: `l+q+p` is 10+1+2, `(q+p)*l` is (1+2)*10. A Float raises the type
so far to Float, so `f+q+l`, with the Float first, keeps every fraction.

The port computes the whole chain in the exact format and rounds once,
when the result is stored: 10+1.5+2.75 is 14.25, stored as 14, and
(1.5+2.75)*10 is 42.5, stored as 42.

The bit operators and shifts take the rounded result of such a chain.
With `p=1.5`:

| Expression | Original | Port |
|---|---|---|
| `r.l=(l+q+p) AND 255` | 12 | 13 |
| `r.l=(l+q+p) LSL 1` | 24 | 26 |
| `r.l=Not (l+q+p)` | -13 | -14 |

A single Quick in a Long computation is computed with its fraction too;
[A Quick in a Long computation]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#quick-long-arithmetic" >}})
shows `r.l=q*2`, `r.l=l/q` and more. The same holds under a Word
destination: `x.w=w+v+q*2` with `w=3`, `v=4` is 9 in the original and 10
in the port.

## A Quick beside a Long {#quick-and-long}

A Long operand, such as a Long variable, a constant above 32767 or the
address of a variable, makes the port compute a Quick operation exactly,
also where nothing is wanted. With `q.q=1.5`:

| Expression | Original | Port |
|---|---|---|
| `NPrint q*100000` | 100000 | 150000 |
| `NPrint Int(q)*100000` | 100000 | 100000 |

The original cuts `q` to 1 when it meets the Long constant. `Int(q)`
gives the original's result in the port as well.

Without a Long, a Quick operation stays a Quick operation with the
original's results, including its overflow: `NPrint i*300` with `i=300`
prints 24464 and `NPrint q+20000` with `q=20000` prints -25536 in both.

## `Mod` with a fraction {#mod}

| Expression | Original | Port |
|---|---|---|
| `x.q=q Mod 2` with `q=7.5` | 1 | 1.5 |
| `NPrint f Mod g` with `f.f=7.5`, `g.f=2` | 1 | 1.5 |

The original computes `Mod` of these operands on their whole parts. The
port computes it exactly whenever an operand has a fraction (a Quick, a
Float or a constant with a point), with the sign rule of the whole
numbers: a negative remainder gets the divisor added. Whole operands keep
the whole-number `Mod`.

{{< callout type="warning" >}}
A `Mod` of two constants is computed by the compiler itself. With a
divisor whose whole part is 0, such as `rq.q=5 Mod 0.5` or
`rq.q=5.25 Mod 0.5` (0 and 0.25), the compiler stops with a "Software
Failure", in the original as in the port. This is an open bug:
[A constant Quick Mod by a divisor below 1 crashes the compiler (#1219)](https://github.com/fubar-coder/bb2-c/issues/1219).
{{< /callout >}}

## Comparisons {#comparisons}

A comparison gives -1 (true) or 0 (false), and only this result takes the
type of the destination. In the original the wanted type also reached the
operands: the left side of a comparison was computed in the destination's
type before the comparison operator was read. With `q.q=1.25`:

| Expression | Original | Port |
|---|---|---|
| `r.l=(q*2>2.4)` | 0 | -1 |
| `r.l=(q+q>2.4)` | 0 | -1 |
| `r.l=(q*2=2.5)` | 0 | -1 |
| `rf.f=(100000001>l3)` with `l3.l=100000000` | 0 | -1 |
| `rq.q=(l1+1>40000)` with `l1.l=40000` | 0 | -1 |

Under the Long destination the original cuts `q` to 1, so `q*2` is 2. Under
the Float destination it turns the Long constant into a Float, which
holds only about seven digits, so 100000001 is not greater than
100000000. Under the Quick destination the Long sum keeps only 16 bits of
its whole part.

The port computes each side of a comparison by its own operands and
compares the two sides in a common type: whole numbers at least as a Long,
a Quick with whole numbers in the exact format, anything with a Float as a
Float. So a Word sum that overflows a Word is compared correctly as well.
With `w.w=20000 : v.w=20000 : c.w=0` and `l1.l=10`:

| Expression | Original | Port |
|---|---|---|
| `rw.w=(w+v>c)` | 0 | -1 |
| `If w+v>c` | false | true |
| `r.l=(l1/3>3.2)` | 0 | -1 |
| `If l1/3>3.2` | false | true |

A comparison whose left side is a single variable is described on
[A comparison with a Quick, or of large Longs]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#quick-compare" >}}):
`r.l=(q>1)` with `q=1.5` is 0 in the original and -1 in the port.

## Bit operators with a fraction {#bit-operators}

`AND`, `OR`, `EOR` and `NOT` work on whole numbers. In the port they take
the whole part of an operand with a fraction, rounded down, on either
side: 1.5 counts as 1, and -1.5 as -2. This holds for a Quick, a value
computed in the exact format and a constant with a point, under every
wanted type and in an array index. A Float variable is refused with
"Illegal Operator for Type", as in the original.

Under a Byte destination the original cut a constant with a fraction to a
whole number towards zero (-1.5 became -1). With `vb.b=3`, and `vb.b=-3`
in the last rows:

| Expression | Original | Port |
|---|---|---|
| `rb.b=vb AND 1.5` | 1 | 1 |
| `rb.b=1.5 AND vb` | 1 | 1 |
| `rb.b=NOT 1.5` | -2 | -2 |
| `rb.b=vb AND -1.5` (`vb=-3`) | -3 | -4 |
| `rb.b=NOT -1.5` | 0 | 1 |

Under a Word, Long or Float type so far the original made such a constant
a Float and refused the operator. With `vw.w=1000`:

| Expression | Original | Port |
|---|---|---|
| `rw.w=vw OR 1.5` | "Illegal Operator for Type" | 1001 |

In an array index after the first one, the original accepted
`ja(1,vq AND 1.5)` only without runtime error checking; the port compiles
it in both builds and reads `ja(1,1)` for `vq=1.5`.

## Shifts {#shifts}

The count of `LSL`, `LSR`, `ASL` and `ASR` is a whole number. A Quick
variable as the count is its whole part, rounded down, in both. A constant
written with a point as the count is refused by the port with "Illegal
Operator for Type", whatever its value. The original accepted it in many
places and shifted by its whole part. With `vb.b=3 : vq.q=1.5`:

| Expression | Original | Port |
|---|---|---|
| `rb.b=vb LSL 2.5` | 12 | "Illegal Operator for Type" |
| `NPrint vb LSL 2.5` | 12 | "Illegal Operator for Type" |
| `rb.b=vq LSL 2.5` | 6 | "Illegal Operator for Type" |
| `NPrint vq LSL 2.5` | 6 | "Illegal Operator for Type" |
| `rb.b=vb ASL 1.0` | 6 | "Illegal Operator for Type" |

Write the count as a whole number (`vb LSL 2`), or use a variable. The
compile error and what to change are also on
[Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors#shift-count-with-point" >}}).

A constant with a point as the value that is shifted is taken as a Quick
variable of that value: `rb.b=1.5 LSL 2` is 6 in the port, where the
original cut the constant first and gave 4.

## `Select` and `Case` {#select}

`Select` evaluates its value once and then compares it with each `Case`
in turn. In the original the value is computed in its own type, as
nothing is wanted, and each `Case` is computed in the type of the value
and compared with it.

The port keeps this for a `Select` whose value is a whole number, a Quick
or a Float: these compile as in the original. A value that the port
computes in the exact format, because a Quick meets a Long, is kept in
that format, and each `Case` is compared with it exactly; a Float `Case`
is compared as a Float. With `q.q=1.5 : l.l=2 : p.q=0.5 : one.l=1 :
f.f=1.5`:

| `Select` | Case lines | The Case line that matches |
|---|---|---|
| `Select q*l` | `Case 3`, `Case 2` | `Case 3` |
| `Select q*one` | `Case 1`, `Case 1.5` | `Case 1.5` |
| `Select q*l+p` with `l=100000` | `Case 150000`, `Case 150000.5` | `Case 150000.5` |
| `Select q*one` | `Case f` | `Case f` |
| `Select q*one` | `Case 2`, `Default` | `Default` |
| `Select q` | `Case 1.5` | `Case 1.5` |
| `Select q+p` | `Case 2` | `Case 2` |

The original matches the same Case line in all of these, so a program
with such a `Select` gives the same results with both. The value 150000.5
is beyond the range of a Quick; the port keeps it exact, so
`Case 150000.5` matches and `Case 150000` does not.

## Edge cases {#edge-cases}

The port's rule has limits, and some special cases are decided on
purpose. Where the original's result is known, it is given.

**Very deep expressions.** The port decides the types over the whole
expression only up to 64 levels of nesting. An expression nested deeper
is computed by the original's rule, from left to right with the type so
far.

**"Expression too Complex".** A value in the exact format takes two of
the processor's eight data registers. An expression that would need a
register beyond the eighth is the compile error "Expression too Complex",
as for the ninth register of
[An operator too deep in an expression]({{< relref "/docs/compared-with-original/new-errors#expression-too-complex" >}}).
With `q.q=1.5 : l.l=2` and the Longs `c1=1`, `c2=2`, `e=3`, `h=4`, `i=5`,
`j=6`:

| Expression | Original | Port |
|---|---|---|
| `r.l=c1+(c2+(e+(h+(i+(q*l*l)))))` | 19 | 21 |
| `r.l=c1+(c2+(e+(h+(i+(j+(q*l*l))))))` | compiles | "Expression too Complex" |

The port finds this only after it has read the whole expression. If the
same expression contains another error further right, such as a string
where a number belongs ("Type Mismatch"), that error is reported first.
An error in a later statement comes after it.

**A whole `Select` with a `Case` with a fraction.** The value and the
Case lines are not brought to a common type: a `Select` of a whole number
compares each `Case` in that whole type, as in the original.

**Other places without a type.** The arguments of functions that take any
number type, such as `Int`, have no wanted type either. A value in the
exact format is passed to them as it would be stored in a Quick variable,
so a value beyond the Quick range wraps there.

**A power.** `^` always works with Floats: both operands are converted to
Float with their own values, and the result is a Float. With `l.l=100000`
and `n.q=2`, `rq.q=l^n` (1e10, far beyond a Quick) gives $FFFFFFFF in the
port, the conversion of a too large Float to a Quick. The original
converted the base to a Quick first and gave $E5400000.

**A constant alone.** A constant on its own keeps the original's rule:
`rb.b=2.0` stores the whole number 2. As an operand a constant with a
point has a fraction, so `rb.b=vb+2.0` with `vb=3` is a Float addition; it
gives 5, as in the original.

**`Mod` by 0.** A `Mod` by a variable that holds 0 behaves by type: a
Byte, Word or Quick `Mod` raises the processor's division trap ("GURU! -
Division by Zero" with runtime error checking), and a Long or Float `Mod`
gives the dividend: `r.l=l Mod z` with `l.l=-7`, `z.l=0` is -7. A `Mod` by
a constant 0 is the compile error "Division by Zero".

**Division by 0 in the exact format.** It gives the largest value of the
wanted type with the dividend's sign. With `l.l=100` and `q=0`:
`x.q=l/q` is $7FFFFFFF, `x.w=l/q` 32767 and `x.b=l/q` 127; `NPrint l/q`
with `l=-100` prints -2147483647. A division of two Quicks by 0 stays the
Quick routine's: `rb.b=q/p` with `q=1`, `p=0` gives -1.

**Products of two constants.** The compiler computes an operation of two
constants itself. A Long or Quick `*` of two constants is still computed
by running the multiplication routine of the loaded libraries, as in the
original, because its result on an overflow depends on that library set.

**Speed.** An operation in the exact format calls a routine of the
compiler's own small library, which is slower than the Quick operation:
an addition takes about 54 cycles of an A500, a multiplication 372 to
1316 and a division 1045 to 3183.

## Developer documentation and issues

The developer documentation of the rule, with the table of every
operation:
[Expression type tables](https://github.com/fubar-coder/bb2-c/blob/main/docs/blitz2/expression-type-tables.md)
and
[the operand typing](https://github.com/fubar-coder/bb2-c/blob/main/docs/blitz2/expression-types.md#the-operand-typing-rule-p-1216).
The fixes:

* [The type of an operation depends on the order of its operands and its place in a chain (#1216)](https://github.com/fubar-coder/bb2-c/issues/1216)
* [A comparison with a compound left operand is still done in the type of the context: a Quick inside it is cut (#1183)](https://github.com/fubar-coder/bb2-c/issues/1183)
* [A comparison with a Quick operand is done in Long when the context or the left operand is Long (#1177)](https://github.com/fubar-coder/bb2-c/issues/1177)
* [Arithmetic with a Quick operand is done in Long when the context or the left operand is Long: the fraction is cut before the operation (#1178)](https://github.com/fubar-coder/bb2-c/issues/1178)
* [A later array index is evaluated as a Word without runtime error checking and as a Long with it (#1179)](https://github.com/fubar-coder/bb2-c/issues/1179)
* [Under a Byte destination a constant with a fraction is cut before the operation (#1188)](https://github.com/fubar-coder/bb2-c/issues/1188)
