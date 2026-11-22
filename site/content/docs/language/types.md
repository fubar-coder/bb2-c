---
title: Types, variables and constants
weight: 1
description: The five number types and the string type of Blitz Basic 2, type suffixes and the default type, number literals, constants, and the Data statement.
prev: /docs/language
---

Every variable of a Blitz Basic 2 program has one type, fixed the first
time the compiler sees it. This page lists the types, shows how a
variable gets its type, how numbers and constants are written, and how
values are kept in `Data` statements.

## The types

There are five number types and the string type:

| Type | Suffix | Range | Steps | Bytes |
|---|---|---|---|---|
| Byte | `.b` | -128 to 127 | 1 | 1 |
| Word | `.w` | -32768 to 32767 | 1 | 2 |
| Long | `.l` | -2147483648 to 2147483647 | 1 | 4 |
| Quick | `.q` | -32768 to 32767.99998 | 1/65536 | 4 |
| Float | `.f` | about ±9.2×10<sup>18</sup> | about 7 digits | 4 |
| String | `$` or `.s` | text of any length | | 4 |

**Quick** is a fixed-point number: a Long that counts in 65536ths. It is
exact for sums and fast, but has only the range of a Word. Quick is the
default type: a variable without a suffix is a Quick unless `DEFTYPE`
says otherwise.

**Float** is the Motorola Fast Floating Point format that AmigaOS's
`mathffp.library` computes with: 24 bits of mantissa, so about seven
decimal digits.

A **string** variable holds a pointer to its text, which can have any
length; the program allocates the memory itself.

The whole-number types wrap around when a result does not fit: a Word
with 32767 plus 1 is -32768, a Byte of 200 is -56. The arithmetic itself
is never checked. With runtime error checking on, the option **Overflow
Checking** of the compiler options adds a check where a value is
converted to a smaller whole-number type, such as a Long assigned to a
Word.

## How a variable gets its type

A variable gets its type the first time it appears in the program, from
its suffix or, without one, from the default type. After that the suffix
can be left out:

```
speed.w=5        ; speed is a Word from here on
speed=speed+1    ; still the Word
```

Writing the variable with another suffix later is the compile error
"Mismatched Types".

**DEFTYPE** has two forms. With a type alone it changes the default type
for every new variable after it; with a list of variables it gives those
variables the type and leaves the default as it was:

```
a=20             ; a Quick
DEFTYPE .f       ; new variables without a suffix are Floats from here
b=20             ; a Float
DEFTYPE .w c,d   ; c and d are Words, the default stays Float
```

The type in `DEFTYPE` can also be a NewType
([NewTypes, arrays and lists]({{< relref "newtypes-arrays-lists" >}})).
The default type also applies to `Data`, to a `Function` without a type
and to the commands with a type suffix, such as `Peek`, when they have
none.

**Strings and the `$` suffix.** A name with `$` is a string variable of
its own: `a$` and the number variable `a` are two variables and can be
used side by side. The `$` must be written every time. The suffix `.s`
instead gives the variable `a` itself the type String; it is then used
without a suffix, and `a$` is again another variable:

```
a$="HELLO" : a.w=20
NPrint a," ",a$          ; 20 HELLO
s.s="abc"
NPrint s                 ; abc
```

In the same way, an array `a()` and a pointer `*a` are variables of
their own next to a plain `a`.

## Assignment

An assignment is `variable=expression`; the word `Let` in front of it
may be written but changes nothing. If an operator follows the variable
instead of `=`, the variable is changed by it:

```
a=10
a+5              ; the same as a=a+5
Let a*2          ; the same as a=a*2: 30
```

The expression is computed in the type of the variable, and the result is
converted to it. A Float becomes a whole number by cutting off the
fraction (-1.5 gives -1), a Quick by rounding down (-1.5 gives -2). The
details are on the page
[From source to executable]({{< relref "/docs/how-blitz2-works/compiling#the-type-of-an-expression" >}}).

`Exchange a,b` swaps the values of two variables of the same type; two
different types are the compile error "Can't Exchange different types".

`MaxLen a$=n` reserves room for n characters in a string variable before
a command writes into it; few commands need it, and their entries in the
command reference say so.

## Numbers

A number in a source can be written in four ways:

| Form | Example | Value |
|---|---|---|
| decimal | `42`, `1.5`, `.5` | 42, 1.5, 0.5 |
| with an exponent | `3e8`, `2.5E-1` | 300000000, 0.25 |
| hexadecimal, `$` | `$FF`, `$dff000` | 255, 14675968 |
| binary, `%` | `%1010` | 10 |

A minus in front of a number is the operator; `-$10` is -16. A number with
a fraction or an exponent is a Float constant, unless the expression it
stands in is computed in Quick. A whole number is the smallest type that
holds it, which matters for the type of an expression
([Expressions and operators]({{< relref "expressions#the-type-of-an-expression" >}})).

## Constants

A constant is a name with `#` in front of it, defined with `=` and a
value that the compiler can compute:

```
#width=320
#height=#width*3/4      ; 240
NPrint #width*2         ; 640
```

A constant costs no memory and no time when the program runs: the
compiler puts its value into the code. Constants have these properties:

* A constant holds a whole number, a Long. A fraction is the compile
  error "Illegal Constant Expression", and a division gives a whole
  number: `#c=10/4` is 2.
* The value must be known while compiling: only numbers, other constants
  and operators. A variable is "Illegal Constant Expression".
* A constant can be defined again with another value further down the
  source; from there on the new value counts.
* A line `#c` alone defines `c` with the value 0.
* Constants can be used in expressions, in `CNIF`
  ([Directives and macros]({{< relref "directives#conditional-compiling" >}})),
  in the definition of other constants and in the inline assembler.

Constants and labels share their names: a label with the name of a
constant is the compile error "Label has been used as a Constant".

The compiler knows the constant `#__debug` itself: it is -1 when the
program is compiled with runtime error checking and 0 without it. It is
an addition of the port
([#__debug: is runtime error checking on?]({{< relref "/docs/compared-with-original/extensions#__debug-is-runtime-error-checking-on" >}})).

**Built-in values.** `Pi` is the Float 3.141592, `On` is -1 and `Off`
is 0. `True` (-1) and `False` (0) are functions of a library
([vallib]({{< relref "/docs/libraries/commands/vallib" >}})).

## Data, Read and Restore

`Data` holds a list of values in the program, which `Read` copies into
variables one after the other. A suffix gives the type of the values;
without one they have the default type. `Data$` holds strings, in quotes
or, without quotes, up to the next comma:

```
Read a,b,c          ; 1 2 3
Restore floats
Read d.f            ; 0.25
Restore texts
Read e$,f$          ; abc  de
End
Data 1,2,3
floats: Data.f 0.25
texts: Data$ "abc",de
```

`Restore label` makes the next `Read` start at the first `Data` after the
label; `Restore` alone goes back to the first `Data` of the program. The
values must be constants. A value that does not fit the type of its
`Data`, such as `Data.b 300`, is the compile error "Bad Data".

`Read` does not convert: the variable must have the type of the value it
reads. With runtime error checking on, a `Read` into a variable of
another type stops the program with "Data type mismatch"; without it,
the variable gets the bytes as they are, so a Quick that reads the Float
0.25 holds nonsense.
