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

## Converting to a whole number: Float and Quick differ

A Float is converted to a Byte, Word or Long by the mathffp routine
`SPFix`, which truncates towards zero: -1.5 gives -1, 1.75 gives 1, -0.5
gives 0. This holds for an assignment, a Long or Word parameter, `Hex$`,
an array index, the position of `Mid$` and `Int(f)`. Float arithmetic is
done in Float and only the result is converted (`r.l=f*2` with `f=-1.5` is
-3). A Float to a Quick truncates towards zero too (-1.5 is `$FFFE8000`,
-0.00001 is 0).

A Quick converted to a whole number is rounded down (towards minus
infinity; inline `clr.w`/`swap`, so -1.5 gives -2), whereas `Int(q)`
truncates towards zero.

Measured with `f.f=-1.5` or `q.q=-1.5` (V in the table), `l.l=3`, `sl{}`
and `sw{}` printing a Long and a Word parameter; blitz2 2.10, 1.95 and the
port with the CD's libraries give the same values, with runtime error
checking off and on:

| Expression | Float -1.5 | Quick -1.5 |
|---|---|---|
| `r.l=V` | -1 | -2 |
| `r.l=-V` | 1 | 2 |
| `x.w=V` / `b.b=V` | -1 | -2 |
| `x.w=-V` | 1 | 1 |
| `r.l=V*2` | -3 | -4 |
| `r.l=V/2` | 0 | -1 |
| `r.l=l*V` | -4 | -6 |
| `r.l=l/V` | -2 | -1 |
| `r.l=l+V` | 1 | 1 |
| `r.l=Int(V)` | -1 | -1 |
| `Hex$(V)` | FFFFFFFF | FFFFFFFE |
| `sl{V}` / `sw{V}` | -1 | -2 |
| `r.l=a(V+3)` (`a(i)=10*i`) | 10 | 10 |

Sources: `eval.c:443-477` (`floattobyte`, `floattoword`, `floattolong`,
`floattoquick`; `blitz2.s:8652-8680`) and `eval.c:404-421`
(`quicktob2`, `quicktoword`, `quicktolong`; `blitz2.s:8624-8634`);
`Int` in `libs/acidlibs/vallib.c:517-541`. `x.w=-V` agrees because the
Quick is negated as a Quick and 1.5 rounds down to 1. The two types differ
a second time through the cut of a Quick operand in a Long context
(above), which a Float does not have: `r.l=V*2` with 1.5 gives 3 for a
Float and 2 for a Quick.

Planned in #1178: a Quick division rounds towards zero, and every other
Quick result and conversion rounds down. This is not the current
behaviour.

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

## Array size and index limits

A dimension of an array holds at most 65535 elements, the highest index
is 65534; the number of dimensions and the total size are limited only by
4 GB (#58). The manual states no limit. The original has these limits and
the port keeps them: a `Dim` or an index outside them is a programming
error, and the check for it exists (runtime error checking). Measured with
2.10 and 1.95 of the CD, and the port, with the CD's and the C libraries:
the four give the same results, except for `Dim a(65535)` with checking
(below).

* **`Dim`** (`dimwit`, `blitz2.s:14571-14610`; port
  `tokens2.c:1449-1480`) multiplies the size so far by the count of each
  dimension with `arrmult` (`bblibsrc/arrayslib:38-39`; port
  `libs/acidlibs/arrayslib.c:162-176`), which does `ADDQ #1,d1` and two
  `MULU d1`: the count is a word. Without checking, `Dim a.l(70000)` gets
  17860 bytes (4465 elements, 70001 mod 65536), `Dim a.l(65535)` gets 0
  bytes and no error is shown; nothing keeps the program from writing
  behind the block. With checking, `arrchk` (`blitz2.s:14591`) refuses
  65536 and more at the `Dim` with "Array Dimension out of Range"
  (`bblibsrc/runerrlib:162`); 65535 is refused only by the port's C
  library (`libs/acidlibs/runerrlib.c:451-454`, #1037), the CD's library
  passes it and the first access is "Array Subscript out of Range".
* **Index** (`calcvar`, `blitz2.s:8214-8290`; port `parse.c:1340-1385`):
  the first index is multiplied with the element size by `muld1`
  (`blitz2.s:11106-11137`; port `libcall.c:603-630`): a shift for a power
  of two (the whole Long counts, so an index of 65536 and more addresses
  behind the array), `mulu` otherwise (only the low word counts: the index
  wraps modulo 65536 into the array). A later index is multiplied by
  `makelmul` (`blitz2.s:8084-8166`; port `parse.c:1280-1295`): `mulu` of
  the low word of the index, unsigned, so it wraps modulo 65536 too.
  Indexes 32768 to 65534 are valid in every dimension (written and read
  back, measured). With checking every index is compared with the size of
  its dimension (`zarrchk`, `blitz2.s:8077`) and an index of 65536 or more
  is "Array Subscript out of Range" (`arrszchk`, `blitz2.s:8234`).
* A Word variable with 32768 to 65535 in it is negative as a first index of
  an array of a power-of-two element size (a wrong address without
  checking, "Array Subscript out of Range" with it), and the right element
  as a later index or with another element size (without checking); its
  value is outside the type's range, so this is a programming error too.

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
