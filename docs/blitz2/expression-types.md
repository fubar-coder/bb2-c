# The type of an expression in blitz2

`blitz2` (like the original compiler, `blitz2.s`) gives an expression one
type while it compiles it from left to right. The rule is not described in
the manual, and it has consequences when a Quick (a number with a fraction
in 16.16 format) meets a Long. This page describes what the compiler does
today, measured with blitz2 1.95 and the CD's libraries with runtime error
checking off (AmiBlitz3 v3.14.0 has the same code).

## The types and the "type so far"

A value has one of the types Byte, Word, Long, Quick, Float, in this order
(the compiler numbers them 1 to 5). While an expression is compiled, the
compiler keeps one **type so far**. Every variable or function value that
is fetched is converted up to the type so far if that is higher; if the
value's own type is higher, the type so far becomes that type (promotion).
The result of the expression has the type so far at the end.

The type so far starts as the type the expression is wanted in:

| Expression | Starts as |
|---|---|
| right side of an assignment | the type of the destination (`r.l=...`: Long) |
| a parameter of a command or function | the type of the parameter (`Poke.l`, the position and length of `Mid$`, the colour of `Boxf`, every parameter of an OS call `Name_`: Long) |
| the first index of an array | Long, always |
| a later index of an array | Long with runtime error checking on, Word with it off |
| `If`, `While`, `Until`, `Print` | none: the first value sets it |

## The exception: a Quick is cut to Long

If the type so far is Long and the value is a Quick, the Quick is converted
to Long at once and its fraction is cut off
(`varcont`, `blitz2.s:10491-10498`; `programs/blitz2/src/eval.c:2894`).
Without it the Quick (4) would win over the Long (3), and the Long operands
would be computed in Quick with a range of +-32768: `adr.l=base+i*4` with a
default-typed `i` would break for addresses.

The cut is towards minus infinity (`-1.5` becomes `-2`). It applies to a
Quick variable and to the value of a Quick function. Floats and constants
with a fraction are not cut: in a Long context they make the type so far
Float.

The type so far is Long when

* the destination or the parameter is a Long (table above),
* an operand to the left is a Long (a Long variable, `Peek.l`, `&var`,
  `?label`, a constant above 32767),
* an operation before made it Long: Word+Word and Word*Word give a Long
  (`doplusw`, `dotimesw`), and so does a shift of a Byte, Word or Long
  (`shpoke`); Byte+Byte gives a Word (`eval.c:1196-1210`,
  `eval.c:1282-1288`, `eval.c:1431-1447`),
* a whole-number constant that does not fit a Quick stands in a Quick
  expression (it becomes a Long, `consttype2`, `libcall.c:420-470`).

## A comparison starts again

A comparison gives a Word (`cpoke`, `eval.c:1087-1093`), so the operands
after it start again: in `If l=5 OR q*2>3` the `q*2` is computed in Quick.
The type so far before the comparison still reaches its operands, which is
a problem (see below).

## Measured examples

`q.q=1.5`, `h.q=0.5`, `l.l=10`, `w.w=3`, `v.w=4`:

| Expression | Result | Mathematical value |
|---|---|---|
| `r.l=q*2` | 2 | 3 |
| `r.l=l-q` | 9 | 8 |
| `r.l=q/h` | 2147483647 (`h` is cut to 0) | 3 |
| `r.l=2.0*q` | 3 | 3 (a Float is not cut) |
| `t.q=q*2 : r.l=t` | 3 | 3 (the Quick is stored first) |
| `NPrint 1+1+q` | 3.5 | 3.5 |
| `NPrint 1+1+1+q` | 4 | 4.5 (Word+Word is a Long) |
| `x.w=w+v+q*2` | 9 | 10 |
| `x.w=q*2+w+v` | 10 | 10 (the order matters) |
| `If l+q*2>12` | false | true |
| `If q*2+l>12` | true | true |
| `a(q*10)` (first index) | element 10 | element 15 |
| `Mid$("abcdefghij",q*2,1)` | b | c |
| `r.l=(q>1)` | 0 | -1 |

## Where this is a problem

* **Comparisons** are original bug #1177 (to be fixed): the operands of a
  comparison are to be compared with their own values, and only the boolean
  takes the type of the context. Today `r.l=(q>1)` is 0 and `r.l=(q=1)` is
  -1 for `q=1.5`, and `q>l` differs from `l<q`.
* **Arithmetic** is issue #1178: the current behaviour is kept, because the
  rule is deliberate; it is recorded as a potential problem, since a
  program that uses fractions in such an expression gets another result
  than its source suggests, depending on the order of the operands.
* **Array indexes** are a separate issue: the same source gives another
  element depending on a compiler option (`b(1,q*10)` is element 15 with
  runtime error checking off; with it on the index is a Long and the
  element would be 10, derived from the code and not measured).

The comment at `programs/blitz2/src/eval.c:2893` points here.
