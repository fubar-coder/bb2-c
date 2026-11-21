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

A comparison gives a Word (`cpoke`), so the operands after it start again:
in `If l=5 OR q*2>3` the `q*2` is computed in Quick.

In the port the operands of a comparison (`=`, `<>`, `<`, `<=`, `>`, `>=`)
are compared with their own values (fix of #1177; the original handed the
type so far to both): the right operand starts with no type, a left
operand that is one element (a variable, field, array element, function
value) gets its own type back, and only the boolean is converted to the
type of the context. A Long or Word against a Quick is compared exactly:
the whole number against the whole part of the Quick, on equality the
fraction decides; a Long is not converted to Quick (range +-32768). So
`r.l=(q>1)` is -1 for `q=1.5`, `q>l` and `l<q` agree, `rq.q=(lw>1)` with
`lw.l=65537` is -1, and two Longs under a Float destination are compared
as Longs (`rf.f=(l2=l3)` is 0 for 100000001 and 100000000, which are the
same FFP value).

Limits until the expression tree (#1180):

* a left operand that is an expression keeps the type so far: in
  `r.l=(q*2>2)` the `q` is still cut (0), and so is `-q` in
  `r.l=(-q<-1)` (#1183);
* a Long constant on the left under a Float context is still an FFP value:
  `rf.f=(100000001>l3)` is 0;
* the right operand is computed in its own types: in `If base<i*4+base`
  (`base.l=$40000`, a default-typed `i=20000`) `i*4` is a Quick and
  overflows (false; the original computed it in Long, true), and a
  constant with a fraction on the right is a Float (`If q>1.4` is a Float
  comparison);
* a comparison that is the right operand of another operator lowers that
  operator's left operand to Word (`rl.l=lw AND (l1=l1)`): issue #1182,
  not part of this fix.

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
| `r.l=(q>1)` | 0 (the port: -1, #1177) | -1 |
| `If q>b` (`b.l=65537`) | true (the port: false, #1177) | false |

## Where this is a problem

* **Comparisons** were original bug #1177, fixed in the port (above, "A
  comparison starts again", with its limits): in the original
  `r.l=(q>1)` is 0 and `r.l=(q=1)` is -1 for `q=1.5`, `q>l` differs from
  `l<q`, a Long against a Quick keeps only its low word
  (`If q>b` is true for `b.l=65537`), and two Longs beyond 2^24 under a
  Float destination or parameter, or two Longs beyond 16 bits under a
  Quick destination, can compare as equal.
* **Arithmetic** is issue #1178: the current behaviour is kept, because the
  rule is deliberate; it is recorded as a potential problem, since a
  program that uses fractions in such an expression gets another result
  than its source suggests, depending on the order of the operands.
* **Array indexes** are original bug #1179: a later index is evaluated as
  a Word without runtime error checking and as a Long with it, so the same
  source gives another element depending on that option (`b(1,q*10)` is
  element 15 with the checking off and element 10 with it on, measured
  with 1.95, 2.10 and the port).

The comment at `programs/blitz2/src/eval.c:2893` points here.
