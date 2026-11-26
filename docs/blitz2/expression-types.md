# The type of an expression in blitz2

`blitz2` (like the original compiler, `blitz2.s`) gives an expression one
type while it compiles it from left to right. The rule is not described in
the manual, and it has consequences when a Quick (a number with a fraction
in 16.16 format) meets a Long. This page describes what the original
compiler does, measured with blitz2 1.95 and the CD's libraries with
runtime error checking off (AmiBlitz3 v3.14.0 has the same code), and
where the port differs: comparisons (#1177), the arithmetic of a
Quick under a Long (#1178), the later array index (#1179), a constant
with a fraction under a Byte (#1188), the power with a negative base
(#1189), the bit operators under a Quick type so far (#1209), and the
order of the operands: the port types every operation by its own
operands and the wanted type ([The operand
typing](#the-operand-typing-rule-p-1216), #1216, with the left operand
of a comparison, #1183), original bugs fixed in the port.

Every rule as a table (each operator with each pair of operand types under
each wanted type, and the constants): [Expression type
tables](expression-type-tables.md), a page generated from the compiler's
type rules.

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
| an index of an array, the first and every later one | Long, always (a later index in the original: Long with runtime error checking on, Word with it off, #1179) |
| `If`, `While`, `Until`, `Print` | none: the first value sets it |

## The exception: a Quick is cut to Long (the original)

In the original, if the type so far is Long and the value is a Quick, the
Quick is converted to Long at once and its fraction is cut off
(`varcont`, `blitz2.s:10491-10498`). Without it the Quick (4) would win
over the Long (3), and the Long operands would be computed in Quick with a
range of +-32768: `adr.l=base+i*4` with a default-typed `i` would break
for addresses.

The cut is towards minus infinity (`-1.5` becomes `-2`). It applies to a
Quick variable and to the value of a Quick function. Floats and constants
with a fraction are not cut: in a Long context they make the type so far
Float. The operation is then done with the whole number, so `r.l=q*2`
gives 2 for `q=1.5`, and the result depends on the order of the operands.

## The port: a Quick keeps its fraction (#1178)

In the port (original bug #1178, fixed; spec `compiler/expression-types`)
the Quick is not cut when it is fetched: the type so far stays Long, the
Quick stays a 16.16 value in its register (`varcont`, the flag `rawq` in
`programs/blitz2/src/eval.c`), and the operator that takes it computes
with the fraction and gives a whole number, rounded at the operator:

| Operation under a Long type so far | Result |
|---|---|
| `+`, `-` (`l+q`, `l-q`, `q-l`, `q+p`) | rounded down: `r.l=l-q` with `l=10`, `q=1.5` is 8 |
| `*` (`l*q`, `q*p`) | rounded down, the product modulo 2^32 beyond 32 bits as the Long product: `r.l=q*2` is 3 |
| `/` (`l/q`, `q/l`, `q/h`) | towards zero, as BB2's Long, Word and Quick divisions: `r.l=l/n` with `n=-1.5` is -6, `r.l=g/h` with -3.0 and 2.0 is -1 |
| unary minus | the Quick negated with its fraction, rounded down where it is converted: `r.l=-q` is -2, `r.l=-q*2` is -3 |
| `^`, an operand that makes the type Float | the Quick with its fraction converted to Float, the Float result to the whole number towards zero (`SPFix`): `r.l=q+f` with `f=0.5` is 2 |
| shifts, `AND`, `OR`, `EOR`, `Not`, the bit operators | the Quick rounded down first, as in the original |
| `MOD` | the Quick rounded down first with a whole-number divisor, with its fraction when the divisor is a Float |
| the end of the expression (an assignment, a parameter) | rounded down, as in the original |

A Long operand that makes the type Long itself after a Quick (a constant
beyond the Quick range, `&var`, `?label`) is a Long operation on the
Quick rounded down for `+`, `-` and the other operators, as in the
original, and takes the fraction for `*` and `/`: `NPrint q*100000` gives
150000 (the original 100000), `NPrint q+&l-&l` gives 1; `Int(q)` gives the
original's result. For a Quick without a fraction every result is the
original's (as long as no Long overflows). Since the operand typing
(#1216, below) a chain of such operations is computed exactly and rounded
once, where its value is taken (`r.l=(q+p)*l` is 42 for `q=1.5`, `p=2.75`,
`l=10`), and the order of the operands does not matter: `NPrint 1+1+1+q`
gives 4.5, `NPrint w+v+q` and `NPrint q+w+v` 8.5.

The products and the division `l/q` need more than 32 bits (`l*q`: bits
16..47 of the 64 bit product; `l/q`: `l` shifted by 16 bits divided by the
Quick): they are three routines of the port's internal library
`compilerlib` (library 64030, `libs/compilerlib`), which `blitz2` brings
itself and links in both modes whatever library set it loads; a program
without such an operation gets nothing of it. `q/l` and `q/h` use the
library set's `longdiv`.

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

## The operand typing (rule P+, #1216)

In the original an expression has one type so far from left to right
(above), so the type of an operation depends on the place of its
operands in the expression and on the operand before it: `NPrint
1+1+1+q` is 4 and `NPrint q+1+1+1` 4.5, `w/2` beside a Quick is the
whole-number division before it and the fractional one after it.

In the port (original bug #1216, fixed with #1183; spec
`compiler/expression-types`, rule P+ of the OpenSpec change
`expr-tree-5-operand-typing`) a typing pass over the finished tree of the
expression (`programs/blitz2/src/exprtyping.c`) types every operation by
its operator, the types of its two operands and the *wanted type* (the
destination's or parameter's type, Long for an index, none for `If`,
`While`, `Until`, `Print`, `Select` and the operands of a comparison),
and the code is written from those decisions (the errors are still
found while the expression is read, as in the original):

* whole numbers are computed in their own types (Byte+Byte a Word as the
  original's handler gives it), a whole-number operation in a comparison
  at least as a Long;
* a Quick with a Quick or a whole number up to a Word is a 16.16
  operation with the original's routines; a Quick with a Long, and every
  operation with a Quick under a wanted Long, is a 48.16 operation: a
  whole part of 48 bits and 16 bits of fraction in a register pair,
  computed exactly by the routines of the port's `compilerlib`
  (`q48add` to `q48trunc`); a Float operand makes the operation Float;
* the value is rounded once, where it is taken: a wanted whole number its
  whole part rounded down (a `/` at the top under a wanted Long towards
  zero), a wanted Quick its 16.16 value, no wanted type a Quick where it
  fits, else its whole part (decided at run time for a 48.16 value);
* `Mod` with an operand that has a fraction (a Quick, a Float, a constant
  with a point) is exact with the whole numbers' sign rule (`NPrint q Mod
  2` is 1.5 for `q=7.5`, `f Mod g` 1.5 for 7.5 and 2); whole operands keep
  the whole-number `Mod`;
* a constant with a point is fractional as an operand, never a whole
  number: a Quick where a Quick is in its expression or wanted, else a
  Float; `AND`, `OR`, `EOR` and `NOT` take its whole part, and the whole
  part of a Quick or 48.16 value (#1209); a constant with a point as a
  shift count is refused;
* a comparison gives a Word, compares its operands exactly in their own
  formats, and makes no Float of the expression around it;
* `^` takes both operands as Floats of their own values;
* a constant subtree is folded by the same rule (`1.5 AND 3` is 1).

So both orders of an operation give the same value: `l+q+p` and `q+p+l`
are 14 for `l=10`, `q=1.5`, `p=2.75` (the original 13), `NPrint
1+1+1+q` 4.5, `w/2+q` and `q+w/2` both use 1.5. The tables of every
operation: [Expression type tables](expression-type-tables.md).

Limits and consequences:

* A tree deeper than 64 levels is typed by the type so far as before (the
  pass recurses).
* A `Select` value that is 48.16 is kept as 48.16 (8 bytes on the stack)
  and each `Case` is compared with it exactly (a Float `Case` in Float); a
  Quick, whole or Float `Select` is as before. The other consumers of a
  value without a type of its own (the arguments of the intrinsics such
  as `Int`) take a 48.16 value as an assignment to a Quick takes it.
* A 48.16 operation takes a register pair, the right operand of an
  operation on it the register after them; one that would need a
  register beyond d7 is "Expression too Complex" (#1185). It is found when
  the whole expression has been read, so an error read later in the same
  expression (a Type Mismatch) is reported first.
* `rq.q=l^n` with `l=100000`, `n.q=2` converts the Float power (1e10)
  to the Quick ($FFFFFFFF, the conversion of a Float beyond the Quick
  range); the original converted the base to a Quick first ($E5400000).
* The compiled code of an expression with a Quick in a chain or beside a
  Long, a `Mod` with a fraction or a constant with a point beside an
  operator changes; a 48.16 operation costs a library call (`q48add` 54
  cycles, `q48mul` 372 to 1316, `q48div` 1045 to 3183 on the A500).

## A constant with a fraction under a Byte type so far (#1188)

A decimal constant is sized by the type so far when it is read
(`consttype2`, `programs/blitz2/src/libcall.c`). Under a Word, Long or
Float type so far a constant with a fraction stays a Float (under a Quick
a Quick), so the operation is computed with the fraction. In the original
the Byte path cut it to a whole number at once (`makeint`,
`blitz2.s:10938`), before the operation: `rb.b=(1.5+1.5)` gave 2,
`rb=(0.5*4)` 0, `rb=(1.5*vb)` 2 for `vb=2`, where a Word destination gives
3, 2 and 3. The same in every Byte context: a Byte parameter of a
statement or function, a Byte field, a Byte array element.

In the port (original bug #1188, fixed; spec `compiler/expression-types`)
a constant with a non-zero fraction under a Byte type so far is a Float
constant, as under a Word: the operation is computed in Float and the
result converted to the Byte at the end (towards zero, as every Float):

| Expression (`vb.b=2`) | Original | Port | Under a Word |
|---|---|---|---|
| `rb.b=(1.5+1.5)` | 2 | 3 | 3 |
| `rb=(0.5*4)` | 0 | 2 | 2 |
| `rb=(1.5*vb)` | 2 | 3 | 3 |
| `rb=(-1.5*vb)` | -2 | -3 | -3 |
| `rb=(1.25e1*2)` | 24 | 25 | 25 |
| `rb=1.5`, `rb=-1.5` | 1, -1 | 1, -1 | 1, -1 |

Where the port differs from a Word type so far:

* a constant alone written with a point or an exponent whose value is
  whole (`rb.b=2.0`, `1e2`, `1.5e1`) stays the whole number it is under a
  Byte, as in the original; as an operand it is fractional since the
  operand typing (#1216): `rb.b=vb+2.0` is a Float addition (5 for
  `vb=3`, as before);
* `Data.b` keeps the cut: `Data.b 1.5` gives 1 (`Data.w 1.5` is "Bad
  Data").

`AND`, `OR`, `EOR` and `NOT` take the whole part of a constant with a
point, as of a variable (#1216): `rb.b=1.5 AND vb` is `1 AND vb`,
`rb.b=NOT 1.5` is -2; a constant with a point as a shift count is refused
("Illegal Operator for Type", `rb.b=vb LSL 2.0`, where the original
computed it with the cut constant). AmiBlitz3 v3.14.0 keeps the fraction
as the port does; it makes a whole-valued constant with a point a Float as
well and refuses `vb AND 2.0`, `1.5 AND vb` and `Data.b 1.5`
(`docs/ab3/variant.md`).

## The power with a negative base (#1189)

`^` converts both operands to Float and calls the first entry of the
math library (`dopow`, `programs/blitz2/src/eval.c`; BB2 mode library 192,
`mathtranslib`), whatever their types; the result is a Float. In the
original that entry is the inline call of mathtrans `SPPow`, which gives
|x|^y for a negative base: the sign was lost also for a whole odd
exponent. `rl.l=vf^nw` with `vf=-2.5`, `nw=3` gave 15, `rf.f=vf^nw`
15.625, `(-1.5)^3` 3.374999 (blitz2 1.95, 2.10, AmiBlitz3 v3.14.0).

In the port (original bug #1189, fixed; spec `compiler/expression-types`)
the entry is a routine (`libs/acidlibs/mathtranslib.c`): a base that is
not negative goes to `SPPow` as before; a negative one goes to it as |x|,
and the result is negative when the exponent's Float value is a whole odd
number. The magnitude is `SPPow`'s, with its precision (`3^2` is
9.000001, `1000^2` 999999: not in scope).

| Expression (`vf.f=-2.5`, `vm.w=-2`, `nw.w=3`) | Original | Port |
|---|---|---|
| `rf.f=vf^nw`, also with a Float or Quick 3 | 15.62499 | -15.62499 |
| `rl.l=vf^nw` | 15 | -15 |
| `rf=vm^2`, `vm^-2` | 4, .25 | 4, .25 |
| `rf=vm^-1`, `vm^-3` | .5, .125 | -.5, -.125 |
| `rf=vm^0` | 1 | 1 |
| `rf=(-1)^16777215` (the largest whole odd Float) | 1 | -1 |
| `rl.l=(-3)^3.5`, `rf=(-3)^1.5` | 46, 5.196152 | 46, 5.196152 |

A fractional exponent has no real power with a negative base; it keeps
the original's |x|^y (whether it becomes a runtime error is an open
question of #1189). A power outside the Long range converts to the
saturated value: `rl.l=vl^3` with `vl=-100000` is -2147483648, 0 as a
Word. AB3 mode takes `^` from AmiBlitz3's library 44 and keeps its value,
the original's (`docs/ab3/variant.md`).

## Bit operators under a Quick type so far (#1209)

When the type so far is Quick (a Quick or default-typed variable as an
operand, a Quick destination or parameter), the original compiles `NOT`,
`AND`, `OR`, `EOR`, `&` and `|` as the Long operation on the 16.16 value
(`notit` emits `not.l`, the Quick column of the operator table has the
Long handlers, `blitz2.s:9688-9698`, `:9496-9521`, `:14725-14745`): the
fraction bits take part. `NOT` inverts them, so the `NOT` of a whole
number has the fraction $FFFF and is almost 1 too high, and the usual
toggle of a default-typed flag does not work; `AND`, `OR` and `EOR`
combine the fractions of both operands.

In the port (original bug #1209, fixed; test
`compiler-quick-bit-operators`) a Quick operand of these operators is converted to a whole number first
by dropping its 16 fraction bits (rounded down, `-1.5` is `-2`, as a
Quick converted to a Word); the result stays a Quick. The code is the
operation on the 16.16 values followed by `clr.w` (`quickwhole`,
`programs/blitz2/src/eval.c`), bit for bit the same as "the operands to
Word, the operation, back to Quick". The type so far flows as before.
With `x=3`, `q.q=1.5`, `p.q=2.5`, `w.w=3`, `l.l=5`:

| Expression | Original | Port |
|---|---|---|
| `h=-1 : h=NOT h`, also `h=True`, `h=(l=5)` | 0.9999847 ($0000FFFF, true) | 0 |
| `h=On : h=NOT h` | 0 | 0 |
| `h=0 : h=NOT h` | -0.0000153 ($FFFFFFFF) | -1 |
| `q2.q=NOT x`, `NOT 3` | -3.0000153 ($FFFCFFFF) | -4 |
| `q2.q=NOT q`, `NOT 1.5` | -1.5000153 ($FFFE7FFF) | -2 |
| `q2.q=NOT -1.5` | 1.4999847 ($00017FFF) | 1 |
| `q2.q=q AND p` | 0.5 | 0 |
| `q2.q=q OR p` | 3.5 | 3 |
| `q2.q=q EOR w` | 2.5 | 2 |
| `If h OR g` with `h=0.5`, `g=0` | true | false |
| `If h OR g` with `h=-0.5`, `g=0` | true | true |
| `q2.q=x AND 6` with `x=13`, `q2.q=y OR 8` with `y=7` | 4, 15 | 4, 15 |

For `AND`, `OR` and `EOR` this is a rule change rather than the repair of
a visibly wrong result: whole operands give the original's value, and
only a fraction differs. A truth test on a fraction between 0 and 1 with
one of these operators is false now (`If h OR g` for `h=0.5`), one on a
fraction between -1 and 0 true (it rounds down to -1). Under a Long type
so far a raw Quick is rounded down before these operators already (#1178,
above). The shifts (`LSL`, `LSR`, `ASL`, `ASR`) keep working on the 16.16
value, as the original does on purpose (`blitz2.s:9555`): `x LSR 1` with
`x=5` is 2.5. AB3 mode has the same rule; AmiBlitz3 v3.14.0 has the
original's (`docs/ab3/variant.md`).

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
checking off and on. The last column is the port after #1178, by its rule
(above; the cases with an operator are checked by the expression matrix
and `compiler-quick-long-arith`):

| Expression | Float -1.5 | Quick -1.5 | Quick -1.5, port |
|---|---|---|---|
| `r.l=V` | -1 | -2 | -2 |
| `r.l=-V` | 1 | 2 | 1 |
| `x.w=V` / `b.b=V` | -1 | -2 | -2 |
| `x.w=-V` | 1 | 1 | 1 |
| `r.l=V*2` | -3 | -4 | -3 |
| `r.l=V/2` | 0 | -1 | 0 |
| `r.l=l*V` | -4 | -6 | -5 |
| `r.l=l/V` | -2 | -1 | -2 |
| `r.l=l+V` | 1 | 1 | 1 |
| `r.l=Int(V)` | -1 | -1 | -1 |
| `Hex$(V)` | FFFFFFFF | FFFFFFFE | FFFFFFFE |
| `sl{V}` / `sw{V}` | -1 | -2 | -2 |
| `r.l=a(V+3)` (`a(i)=10*i`) | 10 | 10 | 10 |

Sources: `eval.c:443-477` (`floattobyte`, `floattoword`, `floattolong`,
`floattoquick`; `blitz2.s:8652-8680`) and `eval.c:404-421`
(`quicktob2`, `quicktoword`, `quicktolong`; `blitz2.s:8624-8634`);
`Int` in `libs/acidlibs/vallib.c:517-541`. `x.w=-V` agrees because the
Quick is negated as a Quick and 1.5 rounds down to 1. In the original
the two types differ a second time through the cut of a Quick operand in a
Long context (above), which a Float does not have: `r.l=V*2` with 1.5
gives 3 for a Float and 2 for a Quick. In the port (#1178) a Quick
division rounds towards zero and every other Quick result and conversion
rounds down; the Float rounds towards zero.

## A comparison starts again

A comparison gives a Word (`cpoke`), so the operands after it start again:
in `If l=5 OR q*2>3` the `q*2` is computed in Quick.

In the port the operands of a comparison (`=`, `<>`, `<`, `<=`, `>`, `>=`)
are compared with their own values (fix of #1177; the original handed the
type so far to both): the right operand starts with no type, a left
operand that is one element (a variable, field, array element, function
value) gets its own type back, so does a constant with a point on the
left (a Float, also under a Byte destination: `rb.b=(1.5<1.9)` is -1,
#1186; since #1188 a constant with a fraction is a Float under a Byte
anyway, a whole-valued one such as `1.0` is made a Float here), and only
the boolean is converted to the type of the context. A Long or Word against a Quick is compared exactly:
the whole number against the whole part of the Quick, on equality the
fraction decides; a Long is not converted to Quick (range +-32768). So
`r.l=(q>1)` is -1 for `q=1.5`, `q>l` and `l<q` agree, `rq.q=(lw>1)` with
`lw.l=65537` is -1, and two Longs under a Float destination are compared
as Longs (`rf.f=(l2=l3)` is 0 for 100000001 and 100000000, which are the
same FFP value).

Since the operand typing (#1216, above) a left operand that is an
operation is typed by its own operands too (#1183): `r.l=(q*2>2.4)` is -1
for `q=1.25`, `rf.f=(100000001>l3)` -1 for `l3.l=100000000`; and both
operands are compared exactly: `If base<i*4+base` (`base.l=$40000`, a
default-typed `i=20000`) and `If q=0.1` (`q=0.1`) are true, as in the
original.

A comparison that is the right operand of another operator (`lw AND
(l1=l1)`, also bound by precedence as in `lw AND l1=l1`, in brackets,
nested, after `NOT` or a minus) gives its boolean, a Word, to that
operator. The original then converted the operator's left operand down
to Word before the operation (`convtypef`, `blitz2.s:8998`): a Long lost
its high word, a Float or a Quick its fraction. In the port (fix of
#1182) the boolean (-1 or 0), or a value computed from it, is converted
up to the left operand's type and the operation is done in that type; a
Byte or Word left operand, a comparison on the left and one passed
through a function are as before. With `lw.l=65537`, `lb.l=100000`,
`fh.f=0.5`, `q.q=1.5`, `qh.q=0.5`, `l1.l=1`:

| Expression | Original | Port |
|---|---|---|
| `rl.l=lw AND (l1=l1)` | 1 | 65537 |
| `rl.l=lb+(l1=l1)` | -31073 | 99999 |
| `If $10000 AND (l1=l1)` | false | true |
| `rf.f=fh+(l1=l1)` | -1 | -0.5 |
| `rq.q=q+(l1=l1)` | 0 | 0.5 |
| `If qh OR (l1<>l1)` | false | false (#1209: the whole part of `qh`) |
| `rl.l=q*(l1=l1)` | -1 | -2 (#1178: rounded down) |
| `rf.f=fh AND (l1=l1)` | 0 | "Illegal Operator for Type" |

`If flag AND x=5` with a default-typed (Quick) `flag` has the same truth
value as in the original for every `flag`: the boolean becomes the Quick
-1.0 and the `AND` takes the whole part of `flag` (as `OR` and `EOR`
do, #1209). A Float has no `AND`, `OR`, `EOR` or shift, so a Float left
operand of one of them with a comparison on the right is refused, as any
Float there. A Long right
operand computed from a comparison under a Quick left one
(`q+(l1=l1)*100000`) keeps the late cut of the Quick (#1178): the Long
may lie beyond the Quick range.

## Measured examples

The measured source of the rules on this page is the expression matrix
([`tests/README.md`](../../tests/README.md), "The expression matrix";
[`tools/exprmatrix`](../../tools/exprmatrix/README.md)): every
expression of two operands (or one) in every context, measured with
2.10, 1.95, the port and AmiBlitz3, with its expected value and its test.
The examples below have more operands than its cells:

`q.q=1.5`, `h.q=0.5`, `l.l=10`, `w.w=3`, `v.w=4`:

| Expression | Original | Port | Mathematical value |
|---|---|---|---|
| `r.l=q*2` | 2 | 3 | 3 |
| `r.l=l-q` | 9 | 8 | 8.5 |
| `r.l=q/h` | 2147483647 (`h` is cut to 0) | 3 | 3 |
| `r.l=2.0*q` | 3 | 3 | 3 (a Float is not cut) |
| `t.q=q*2 : r.l=t` | 3 | 3 | 3 (the Quick is stored first) |
| `NPrint 1+1+q` | 3.5 | 3.5 | 3.5 |
| `NPrint 1+1+1+q` | 4 | 4 | 4.5 (Word+Word is a Long) |
| `x.w=w+v+q*2` | 9 | 10 | 10 |
| `x.w=q*2+w+v` | 10 | 10 | 10 |
| `If l+q*2>12` | false | true | true |
| `If q*2+l>12` | true | true | true |
| `a(q*10)` (first index) | element 10 | element 15 | element 15 |
| `Mid$("abcdefghij",q*2,1)` | b | c | c |
| `r.l=(q>1)` | 0 | -1 (#1177) | -1 |
| `If q>b` (`b.l=65537`) | true | false (#1177) | false |

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
  the low word of the index, unsigned, so it wraps modulo 65536 too (the
  port evaluates a later index as a Long, #1179, and keeps this).
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
* **Arithmetic** was original bug #1178, fixed in the port (above, "The
  port: a Quick keeps its fraction"): in the original a Quick fetched
  under a Long type so far is cut to a whole number before the operation,
  so a program that uses fractions in such an expression gets another
  result than its source suggests, depending on the order of the
  operands.
* **The order of the operands** was original bug #1216, fixed in the port
  with the left operand of a comparison (#1183; above, "The operand
  typing"): in the original the type of an operation depends on its place
  in the expression (`NPrint 1+1+1+q` 4, `NPrint q+1+1+1` 4.5), so the same
  terms in another order give another value.
* **Array indexes** were original bug #1179, fixed in the port (spec
  `compiler/expression-types`): in the original a later index (the second
  and every further one) is evaluated as a Word without runtime error
  checking and as a Long with it (`arrevalchk`, `blitz2.s:8809-8811`), so
  with a Quick operand the same source uses another element depending on
  that option, and the checked build does not report the element the
  unchecked build uses: with a default-typed `y=200` `g(1,y*200/2)` is
  element 52768 unchecked (`y*200` overflows the Quick range) and 20000
  checked, `b(1,q ASL 4)` with `q=1.5` element 24 against 16 (measured
  with 1.95, 2.10 and the port before the fix). In the port every index is
  evaluated as a Long by the rules of a Long evaluation (above, #1178),
  with and without the checking: both builds use element 20000 and 16,
  and `b(1,q*10)` is element 15 in both. A shift or bit operator of a
  Quick and a constant with a fraction in a later index is typed as in
  the first index and the checked build: `ja(1,vq AND 1.5)` takes the
  whole part of the constant (#1216), a shift count with a point is
  refused ("Illegal Operator for Type").
  Without the checking the size check stays off: an index above 65535
  still uses its low word (`g(1,l4*2)` with 80000 is element 14464).
  The code: without the checking only the low word of a later index is
  read (`makelmul`), so the upper word is not built: a constant index is
  loaded with `move.w`, and the final sign extension of an index that is
  one Byte, Word or Quick element is left out; `a(i,j)`, `b(i,j,k)` and
  `a(5,7)` keep the original's code byte for byte. A `*` or `/` of two
  Word values (Byte or Word variables and function values, constants 0 to
  32767) is computed inline in both builds (`muls.w`; `divs.w` with a
  fall-back that gives 32768 for -32768 / -1) instead of calling `lmullib`
  or `ldivlib`. The cost, A500 cycles per access against the original's
  unchecked code (`tests/blitz2/array-index-code` pins the code): an
  index of one element 0, one with `+`, `AND` or a shift +4 to +13, a
  Word `*` +9, a Word `/` +19 (the fall-back), a Quick `*` about 180 to
  220 cycles faster (the exact Long multiplication of #1178, which takes
  one or two `MULS` when a factor is a word or a whole Quick, #1191:
  `c=a(iq,jq*4+kq)`, `c=a(iq,jq*kq)`); with the checking a Word `*` is 140
  and a Word `/` about 400 cycles faster. A Word division by 0 in a later
  index stops in the CPU's division trap without the checking (as the
  original's Word division there), where a Long division gives the
  largest number (`ldivlib`, #200); with the checking it stops with
  "Division by Zero" in both (`tests/blitz2/array-index-div0`).
* **Constants with a fraction under a Byte** were original bug #1188,
  fixed in the port (above, "A constant with a fraction under a Byte type
  so far"): in the original `rb.b=(1.5*vb)` computes `1*vb`, so a Byte
  destination, parameter, field or array element gives another result
  than the same expression under a Word.
* **The power with a negative base** was original bug #1189, fixed in
  the port (above, "The power with a negative base"): in the original
  `(-2.5)^3` is 15.625, so an odd power of a negative number has the
  wrong sign.
* **Bit operators under a Quick type so far** were original bug #1209,
  fixed in the port (above, "Bit operators under a Quick type so far"):
  in the original `h=True : h=NOT h` leaves `h` true (0.9999847), so the
  usual toggle of a default-typed flag does not work, and `NOT 3` is
  -3.0000153.

The comments `Fixed (#1178)` and `Fixed (#1179)` in
`programs/blitz2/src/eval.c` (`contemit`, `mixedop`, `arrevalchk`),
`Fixed (#1183)` there (`ownleft`) and
`Fixed (#1188)` in `programs/blitz2/src/libcall.c` (`consttype2`) and
`Fixed (#1189)` in `programs/blitz2/src/eval.c` (`dopow`) and
`libs/acidlibs/mathtranslib.c` and `Fixed (#1209)` in
`programs/blitz2/src/eval.c` (`quickwhole`, `notit`) point here.

## Where the rules are in the code

The operand typing (#1216) is the typing pass
`programs/blitz2/src/exprtyping.c` over the tree the evaluator records
while it builds the expression (`p5end` in `programs/blitz2/src/eval.c`,
at the end of `evalbuild`): where its decision is the code the build
wrote, that code stays; otherwise the code is rewritten from the
decisions (`exprarith.c` and `exprfold.c` fold the constants). The pass
is pinned by the host test `blitz2_exprtyping` against the expression
matrix's model (`EM_RULES_DECIDED`). The flow of the type so far, which
the build still uses for the errors and the strings, is taken in one
place, `programs/blitz2/src/exprtype.c` (issue #1180, step 1 of the
expression tree): `exprtype_binop` for a binary operator (the conversions of its
operands, the type whose handler computes it, the type so far after it;
the rules of #978, #1177, #1178, #1179 and #1182 included),
`exprtype_element` for a variable, a function value or a constant (the
type so far after it; `varcont` and `consttype2`'s sizing, #1188
included), and `exprtype_optype` for the type each operator handler
gives. The evaluator (`exprtree_pre`, `operate`, `doop`, `compareop`, `mixedop`,
`ownleft`, `binopbuild`, `contleaf`, `consttype2`) asks them and emits what they
return. Their decisions are pinned by the host test `blitz2_exprtype`
(`programs/blitz2/tests/host/test_exprtype.cpp`) against the committed
table `programs/blitz2/tests/host/exprtype.tsv`, which lists every
operator, type so far, operand type and operand fact, and against the
expression matrix's model for its two-operand cells. The tables of
[expression-type-tables.md](expression-type-tables.md) are generated from
the rule of the pass and these functions by `tools/exprtables`; the build
checks that the committed page is up to date. Which routine of the original each rule comes from:
[expression-tree.md](expression-tree.md).
