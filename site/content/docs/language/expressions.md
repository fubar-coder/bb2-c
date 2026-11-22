---
title: Expressions and operators
weight: 3
description: The operands of an expression, the operators of Blitz Basic 2 in their order, comparisons and truth values, addresses, and the type in which an expression is computed.
---

Before this page you should know the types
([Types, variables and constants]({{< relref "types" >}})).

An expression is a calculation that gives one value: `a*7+3`,
`Left$(n$,2)+"..."`, `x>5 AND y<10`. This page lists what an expression
can contain, the operators in the order the compiler applies them, and
the rule that decides in which type the expression is computed.

## Operands

An expression is built of operands joined by operators:

* numbers (`42`, `1.5`, `$FF`, `%101`) and strings in double quotes
  (`"Hello"`);
* variables, array elements and fields (`a`, `a$`, `a(3)`, `p\x`,
  `*p\x`);
* constants (`#width`) and the built-in values `Pi`, `On` and `Off`;
* functions of the libraries, with their arguments in brackets
  (`Sqr(2)`, `Left$(a$,2)`, `Event`), and your own functions with their
  arguments in curly brackets (`fact{5}`,
  [Procedures]({{< relref "procedures" >}}));
* `SizeOf` of a type or a field
  ([NewTypes, arrays and lists]({{< relref "newtypes-arrays-lists#newtypes" >}}));
* the address of a variable with `&` or of a label with `?`
  ([Addresses](#addresses));
* another expression in round or curly brackets: `(a+1)*2`, `{a+1}*2`.

A string expression joins strings with `+`, and compares them with the
comparison operators. A string and a number cannot be mixed: `"a"*2` is
the compile error "Type Mismatch"; `Str$` and `Val` convert between them.

## The operators

The table lists the operators that join two operands, from the one that
binds most tightly to the one that binds least. Operators in the same row
have the same precedence and are applied from left to right.

| Operators | Meaning |
|---|---|
| `BitTst` `BitSet` `BitClr` `BitChg` | test, set, clear or change the bit of the left operand that the right one numbers |
| `^` | the left operand to the power of the right one |
| `LSL` `ASL` `LSR` `ASR` | shift left (both the same), shift right without and with the sign |
| `&` `\|` `MOD` | bitwise and, bitwise or, remainder of a division |
| `*` `/` | multiplication, division |
| `+` `-` | addition (and joining strings), subtraction |
| `=` `<>` `<` `>` `<=` `>=` | comparisons |
| `AND` `OR` `EOR` | bitwise and, or, exclusive or |

So `7 MOD 3*2` is 2, `1+2 LSL 2` is 9, and `6 AND 3+1` is 4, but
`6 & 3+1` is 3: `&` and `|` bind more tightly than the arithmetic, `AND`
and `OR` less tightly than the comparisons. `1 OR 2 AND 4` is
`(1 OR 2) AND 4`, which is 0. Brackets change the order, as usual.

The comparisons can also be written with their two characters swapped:
`><` is `<>`, `=<` is `<=` and `=>` is `>=`.

Two operators stand in front of one operand:

* **The minus sign** negates the operand right after it, before any
  other operator: `-3^2` is 9, the square of -3. Write `-(3^2)` for -9.
* **`NOT`** inverts every bit of the value of the whole rest of the
  expression: `NOT a=b` is `NOT (a=b)`, and `NOT 5+1` is `NOT 6`, which
  is -7. Put `NOT` in brackets to apply it to one operand only:
  `(NOT a) AND b`.

Some details of the operators:

* `/` of two whole numbers gives a whole number, rounded towards zero:
  `7/2` is 3, `-7/2` is -3. With a Float or Quick operand the result has
  a fraction: `7.0/2` is 3.5. A division by the constant 0 is the compile
  error "Division by Zero".
* `MOD` gives the remainder of the division. With a positive divisor it
  is between 0 and the divisor less 1, also for a negative dividend:
  `10 MOD 3` is 1, `-7 MOD 3` is 2. So with a negative dividend it does
  not match `/`, which rounds towards zero: `-7/3` is -2, and -2*3+2 is
  not -7 ([#1198](https://github.com/fubar-coder/bb2-c/issues/1198)).
  With a negative divisor and a positive dividend the remainder is
  positive: `7 MOD -3` is 1. With both negative it is outside the
  divisor's range, `-7 MOD -3` is -4, a bug of the original that the
  port has too
  ([#1197](https://github.com/fubar-coder/bb2-c/issues/1197)). Of a
  Quick or a Float the remainder is a whole number: `7.5 MOD 2` is 1.
* `^` always computes in Float, also with whole numbers, and the result
  is not always exact: `3^2` is 9.000001.
* The shifts and the bit operators work on whole numbers and Quicks; a
  Float operand is the compile error "Illegal Operator for Type". The
  same holds for `&`, `|`, `AND`, `OR`, `EOR` and `NOT`. `LSR` fills with
  zeros (`-1 LSR 28` is 15), `ASR` with the sign (`-16 ASR 2` is -4).
* `BitTst` gives true or false; `BitSet`, `BitClr` and `BitChg` give the
  left operand with the bit changed: `1 BitSet 3` is 9.

## Truth values

A comparison gives -1 for true and 0 for false, and `If`, `While` and
`Until` take every value that is not 0 as true. Because true is -1, a
value with all bits set, the bitwise operators `AND`, `OR`, `EOR` and
`NOT` also work as the logical ones: `2=2 AND 5=6` is 0, `NOT 0` is -1.

```
NPrint 2=2           ; -1
NPrint 2=2 AND 5=6   ; 0
If a>0 AND a<10 Then NPrint "a digit"
```

Two values that are not truth values do not combine logically: `1 AND 2`
is 0, although both are true for `If`. Compare them with 0 first:
`a<>0 AND b<>0`.

Strings are compared character by character by their codes, so `"B"`
is less than `"a"`, and a string is less than a longer one that starts
with it: `"ab"<"abc"` is true.

## Addresses

`&` in front of a variable gives its address in memory as a Long, and
`?` in front of a label the address of the program's code or data at
that label. They are mostly used with `Peek`, `Poke`, the AmigaOS calls
and the inline assembler:

```
v.l=5
NPrint Peek.l(&v)      ; 5
```

The address of a string variable points to the first character of the
text; its length is a Long at the address less 4. The pointer variables
of [NewTypes, arrays and lists]({{< relref "newtypes-arrays-lists#pointers" >}})
take such addresses.

## The type of an expression

The compiler computes an expression in one of the number types, and it
does not look at the whole expression first. It reads it from left to
right and keeps a *type so far*:

1. The type so far starts as the type the result is wanted in: the type
   of the variable in an assignment, the type of a parameter. In `If`,
   `While`, `Until`, `Select` and `Print` nothing is wanted, and the first
   operand sets the type.
2. A variable or function of a higher type raises the type so far, in
   the order Byte, Word, Long, Quick, Float. The operands before it are
   converted up.
3. A whole-number constant takes the smallest type that holds it, and
   two Bytes added give a Word, two Words added or multiplied a Long.
4. A comparison starts again: its operands are compared with their own
   values, and its result, a truth value, takes the type of the context.

The rule explains results that look surprising:

```
NPrint 7/2           ; 3: two whole numbers
NPrint 7.0/2         ; 3.5
a.w=10 : NPrint a/4  ; 2: a Word divided by a whole number
r.l=2147483647/2*2   ; 2147483646, a Long computation
```

Two parts of the rule need care. In a Long computation, a Quick does not
make the computation Quick: `adr.l=base+i*4` with a Quick `i` is computed
as a Long, as an address must be, and each operation with the Quick is
rounded to a whole number. And a constant with a fraction keeps it while
the type so far is Byte, as it does under a Word: `NPrint 3+1.5` prints
4.5, and with `vb.b=2`, `rb.b=1.5*vb` gives 3. The original cut the
constant to a whole number there and printed 4 and gave 2
([#1188](https://github.com/fubar-coder/bb2-c/issues/1188)). Such a
constant is a Float, so `AND`, `OR`, `EOR`, `NOT` and the shifts refuse
it. A whole-valued constant written with a point, such as `2.0`, stays a
whole number under a Byte.

The page
[From source to executable]({{< relref "/docs/how-blitz2-works/compiling#the-type-of-an-expression" >}})
explains the rule in detail, with the rounding of a Quick in a Long
computation, the comparisons and the array indexes, and where the port's
results differ from the original's.

Based on the Blitz Basic 2 manual by Acid Software, which lists `NOT` and
the minus sign with the highest precedence; the compiler applies the
minus sign to the next operand only and `NOT` to the rest of the
expression, as described above.
