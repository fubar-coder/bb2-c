# compiler/expression-types Specification

## Purpose
Says what an arithmetic operator gives when one operand is a Quick and the
other operand or the type the expression is wanted in is a whole-number
type, and which parts of the typing stay as in the original.

## ADDED Requirements

### Requirement: A Quick operand is computed with its fraction
When `+`, `-`, `*` or `/` has a Quick operand and the other operand or the type so far is a Byte, Word or Long, the compiler SHALL compute it with the Quick's fraction and give a whole number, rounded at the operator: down for `+`, `-`, `*`, towards zero for `/`. This SHALL hold also for two Quicks under a whole-number type so far. The Quick SHALL NOT be cut when it is fetched.

#### Scenario: Long destination
- **WHEN** a program evaluates `r.l=q*2`, `r.l=l-q`, `r.l=l*q`, `r.l=q/h`, `r.l=l+q`, `r.l=q+p` with `q=1.5`, `h=0.5`, `p=2.75`, `l=10`
- **THEN** it gets 3, 8, 15, 3, 11 and 4

#### Scenario: Negative values
- **WHEN** a program evaluates `r.l=n*2`, `r.l=l*n`, `r.l=m/n` and `r.l=l/n` with `n=-1.5`, `l=10`, `m=9`
- **THEN** it gets -3, -15, -6 and -6 (the inexact quotient rounds towards zero)

#### Scenario: Division towards zero in every form
- **WHEN** a program evaluates `r.l=k/q` (`k=-10`, `q=1.5`), `r.l=n/l` (`n=-1.5`, `l=10`), `r.l=g/h` (`g=-3.0`, `h=2.0`) and `r.l=g/l`
- **THEN** it gets -6, 0, -1 and 0

#### Scenario: Whole-number Quick
- **WHEN** the Quick operands have no fraction (`q=3.0`, `q=-3.0` with `l=10`; `q=20000.0` and `p=20000.0` in `r.l=q+p`; `q=300.0` and `p=200.0` in `r.l=q*p`)
- **THEN** every result of an arithmetic operator equals the result of the previous compiler (40000 and 60000 for the last two), as long as no Long result overflows

### Requirement: Negation and conversion round down
Unary minus of a Quick under a whole-number type so far SHALL negate the value with its fraction and round it down; the conversion of a Quick to a Byte, Word or Long (an assignment, a parameter, the end of an expression) SHALL round down as before. `Int(q)` SHALL truncate towards zero as before.

#### Scenario: Negation
- **WHEN** a program evaluates `r.l=-q`, `r.l=0-q`, `x.w=-q` and `r.l=-q*2` with `q=1.5`
- **THEN** it gets -2, -2, -2 and -3

#### Scenario: Conversion
- **WHEN** a program evaluates `r.l=n` and `r.l=Int(n)` with `n=-1.5`
- **THEN** it gets -2 and -1

### Requirement: A Long operand that sets the type itself
When a Quick is followed by a Long operand that makes the type so far Long by itself (a constant beyond the Quick range, `&var`, `?label`) and no type was set before, `*` and `/` SHALL be computed with the Quick's fraction as in "A Quick operand is computed with its fraction"; `+`, `-` and the other operators SHALL be a Long operation on the Quick rounded down, as before. `Int(q)` gives the previous result for `*` and `/`.

#### Scenario: Multiplication by a large constant
- **WHEN** a program prints `NPrint q*100000` and `NPrint Int(q)*100000` with `q=1.5`
- **THEN** it prints 150000 and 100000

#### Scenario: Address operand
- **WHEN** a program prints `NPrint q+&l-&l` with `q=1.5`
- **THEN** it prints 1, as before

### Requirement: Float operands and powers take the fraction
A Quick that meets a Float operand or `^` under a whole-number type so far SHALL be converted to Float with its fraction; the Float result SHALL be converted to the whole-number type towards zero as before.

#### Scenario: Float and power
- **WHEN** a program evaluates `r.l=q+f` and `r.l=q^2` with `q=1.5`, `f=0.5`
- **THEN** it gets 2 and 2

### Requirement: Other operators take the Quick rounded down
Shifts, `AND`, `OR`, `EOR`, `Not` and `MOD` with a whole-number divisor, with a Quick operand under a whole-number type so far, SHALL take the Quick rounded down, as before; `MOD` with a Float divisor takes the Quick with its fraction ("Float operands and powers take the fraction").

#### Scenario: Shift and mask
- **WHEN** a program evaluates `r.l=q ASL 4` and `r.l=t AND 3` with `q=1.5`, `t=-1.5`
- **THEN** it gets 16 and 2, as before

### Requirement: Rounding happens once per operation
A Quick OP Quick, or a Quick expression stored in a Quick, under a Quick type so far SHALL keep the fraction. Under a whole-number type so far a chain SHALL be rounded after each operation, not computed exactly: `r.l=l+q+p` with `l=10`, `q=1.5`, `p=2.75` gives 13 (exactly 14).

#### Scenario: Quick destination keeps the fraction
- **WHEN** a program evaluates `t.q=w+v+q` with `w=3`, `v=4`, `q=1.5`
- **THEN** `t` is 8.5

#### Scenario: Chain
- **WHEN** a program evaluates `r.l=l+q+p` with `l=10`, `q=1.5`, `p=2.75`
- **THEN** `r` is 13

### Requirement: The expected type flows as before
The wanted type (destination, parameter, Long for an array index), a Long operand to the left, and the types Word+Word, Word*Word and a shift give SHALL decide the type of the operations as before; a constant SHALL take that type. The order dependence SHALL stay where the type so far differs. Only the cut at fetch goes away. The operands of a comparison do not take the type (#1177, `quick-compare-types`).

#### Scenario: Word and Long contexts
- **WHEN** a program prints `NPrint 1+1+1+q`, `NPrint w+v+q` and `NPrint q+w+v` with `q=1.5`, `w=3`, `v=4`
- **THEN** it prints 4, 8 and 8.5 (Word+Word is a Long; an interim value until the expression tree, #1180)

#### Scenario: Order cases
- **WHEN** a program evaluates `x.w=w+v+q*2` and `x.w=q*2+w+v` with `q=1.5`, `w=3`, `v=4`
- **THEN** both give 10 (before: 9 and 10)

#### Scenario: Address idiom
- **WHEN** a program evaluates `adr.l=base+i*4` with `i` a default-typed variable holding 20000 and `base` a Long
- **THEN** the result is `base+80000`, as before (the Long operand to the left keeps the computation in Long)

### Requirement: The rule is the same in both compiler modes
blitz2 and blitz2-ab3 (AB3 mode) SHALL apply the same rule; AmiBlitz3 v3.14.0 keeps the old one.

#### Scenario: AB3 mode
- **WHEN** an AB3 source evaluates `r.l=q*2` with `q=1.5`
- **THEN** `r` is 3

#### Scenario: Any library set
- **WHEN** the port compiles `r.l=l*q`, `r.l=q*p` or `r.l=l/q` with its own libraries, with the CD's libraries, or in AB3 mode with AmiBlitz3's libraries
- **THEN** the program compiles and gives the same results: the routines of these operations come with the compiler (its internal library), not from the library set

### Requirement: A mixed product beyond 32 bits wraps
A Long times a Quick whose result does not fit a Long SHALL give the result modulo 2^32, as the Long multiplication does.

#### Scenario: Overflow
- **WHEN** a program evaluates `r.l=l*q` with `l=100000`, `q=30000.5`
- **THEN** `r` is the exact product, rounded down, modulo 2^32

### Requirement: No other result changes
A program that has no Quick operand in an arithmetic expression under a whole-number type so far, and a program whose Quick operands in such expressions hold whole numbers, SHALL give the same results as before the change. Comparisons (#1177) and array indexes (#1179) are not changed by this requirement, except that a compound left operand of a comparison is computed by these rules (#1183 limit).

#### Scenario: Sources unchanged
- **WHEN** the corpus and the CD's sources that compile are compiled and run before and after
- **THEN** the outputs are the same except for the places whose result differs today, listed with the change; every other executable that differs is named for the comparison with the previous commit

## MODIFIED Requirements

### Requirement: Known limits of the left operand until the expression tree
Until the expression tree (#1180), a left operand of a comparison that is an expression (`q*2`, `q+q`) SHALL still be evaluated in the type the expression is wanted in (#1183), by the rules of "A Quick operand is computed with its fraction" (rounded at the operator), and a Long constant on the left under a Float context SHALL still be compared as a Float. A negated Quick on the left (`-q`) SHALL keep its fraction.

#### Scenario: Compound left operand
- **WHEN** a program evaluates `r.l=(q*2>2)` with `q=1.5` and `r.l=(q*2>2.4)` with `q=1.25` (the left operand is an expression)
- **THEN** it gives -1 (3 > 2) and 0 (the operation rounds 2.5 down to 2); `rw.w=(q*2>2.4)` gives -1

#### Scenario: Negated left operand
- **WHEN** a program evaluates `r.l=(-q<-1)` with `q=1.5`
- **THEN** it gives -1

#### Scenario: Long constant on the left under Float
- **WHEN** a program evaluates `rf.f=(100000001>l3)` with `l3.l=100000000`
- **THEN** it gives 0 as before (both are the same FFP value)
