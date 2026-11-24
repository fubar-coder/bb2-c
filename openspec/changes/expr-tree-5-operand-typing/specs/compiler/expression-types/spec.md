# Spec Delta

## ADDED Requirements

### Requirement: The type and format of an operation do not depend on the order of its operands
The compiler SHALL give each operation its type and its format (whole number, 16.16, 48.16, Float) from its operator, its operands' types and the wanted type, never from the order of its operands or the place of a subexpression in a chain. The values in both orders SHALL agree unless an intermediate value leaves its format (an overflow). The rule SHALL be the same in AB3 mode.

#### Scenario: Base plus scaled Quick
- **WHEN** a program evaluates `adr.l=base+q*4` and `adr2.l=q*4+base` with `base.l=2097152`, `q=1.5`
- **THEN** adr and adr2 are both 2097158

#### Scenario: Chain of Words and a Quick under a Word destination
- **WHEN** a program evaluates `x.w=w+v+q*2` and `y.w=q*2+w+v` with `q=1.5`, `w=3`, `v=4`
- **THEN** x and y are both 10

#### Scenario: Comparison of a chain
- **WHEN** a program evaluates `If l+q*2>12` and `If q*2+l>12` with `l=10`, `q=1.5`
- **THEN** both conditions are true

#### Scenario: Chain of a Long and Quicks
- **WHEN** a program evaluates `r.l=l+q+p` and `r.l=q+p+l` with `l=10`, `q=1.5`, `p=2.75`
- **THEN** both give 14 (today 13 and 14)

#### Scenario: Bit operator, shift and Not of a mixed chain
- **WHEN** a program evaluates `r.l=(l+q+p) AND 255` and `r.l=(q+p+l) AND 255`, `r.l=(l+q+p) LSL 1` and `r.l=(q+p+l) LSL 1`, `r.l=Not (l+q+p)` and `r.l=Not (q+p+l)` with `l=10`, `q=1.5`, `p=1.5`
- **THEN** they give 13 and 13, 26 and 26, -14 and -14 (today 12 and 13, 24 and 26, -13 and -14)

#### Scenario: A Float in the chain
- **WHEN** a program evaluates `r.l=l+q+f` and `r.l=f+q+l` with `l=10`, `q=1.5`, `f.f=0.5`
- **THEN** both give 12 (today 11 and 12)

#### Scenario: Overflow of a Word sum in both orders
- **WHEN** a program executes `NPrint w+v+q` and `NPrint q+w+v` with `w=20000`, `v=20000`, `q=0.5`
- **THEN** both print -25535.5 (the 16.16 sum wraps the same way in both orders)

#### Scenario: The limit: an intermediate leaves its format
- **WHEN** a program evaluates `x.q=q*q2*r` and `x.q=r*q*q2` with `q=300`, `q2=300`, `r=0.25`
- **THEN** the first gives 6116 (the 16.16 product 90000 wraps) and the second 22500; both are typed alike

#### Scenario: AB3 mode
- **WHEN** an AB3 source evaluates `r.l=(l+q+p) AND 255` and `r.l=(q+p+l) AND 255` with `l=10`, `q=1.5`, `p=1.5`
- **THEN** both give 13, as in blitz2 (AmiBlitz3 v3.14.0 keeps the order dependence)

### Requirement: The left operand of a comparison is compared with its own value whatever its shape
The left operand of a comparison SHALL be typed by its own operands, not by the type the expression is wanted in, also when it is an expression, a negation or a constant, and the comparison SHALL be exact for the two operand types. A whole-number operation inside the operands of a comparison SHALL be computed at least as a Long.

#### Scenario: Compound left operand with a Quick
- **WHEN** a program evaluates `r.l=(q*2>2.4)` with `q=1.25`
- **THEN** r is -1

#### Scenario: Sum of Quicks on the left
- **WHEN** a program evaluates `r.l=(q+q>2.4)` with `q=1.25`
- **THEN** r is -1

#### Scenario: Negated Quick on the left
- **WHEN** a program evaluates `r.l=(-q<-1.25)` with `q=1.5`
- **THEN** r is -1

#### Scenario: Long constant on the left under Float
- **WHEN** a program evaluates `rf.f=(100000001>l3)` with `l3.l=100000000`
- **THEN** rf is -1 (today 0)

#### Scenario: Long sum on the left under a Quick destination
- **WHEN** a program evaluates `rq.q=(l1+1>40000)` with `l1.l=40000`
- **THEN** rq is -1 (today 0)

#### Scenario: Whole-number sums and products on the left keep their values
- **WHEN** a program evaluates `r.l=(w+v>c)` and `r.l=(w-(-v)>c)` with `w=20000`, `v=20000`, `c=0`, `rw.w=(w+v>c)` and `If w+v>c` with the same values, `r.l=(q*1000>50000)` with `q=100`, and `rb.b=(b1+b2>150)` with `b1.b=100`, `b2.b=100`
- **THEN** r is -1 and -1, rw is -1, the condition is true, r is -1 and rb is -1

### Requirement: A Quick keeps its fraction when nothing asks for a whole number
When no destination, parameter or operand asks for a whole-number type, an expression with a Quick operand SHALL give the Quick value with its fraction.

#### Scenario: Constants plus a Quick in Print
- **WHEN** a program executes `NPrint 1+1+1+q` with `q=1.5`
- **THEN** it prints 4.5

#### Scenario: Word sums and products beside a Quick
- **WHEN** a program executes `NPrint w+v+q`, `NPrint q+w+v` and `NPrint w*v+q` with `q=1.5`, `w=3`, `v=4`
- **THEN** it prints 8.5, 8.5 and 13.5

### Requirement: The wanted type and a Long set the format of the intermediate results
In an expression with a Quick, `+`, `-`, `*`, `/` and `Mod` SHALL be computed in 48.16 where a Long is wanted or takes part (a Long value, a constant beyond a Word, `&var`, a Word*Word product, a shift of a whole number, a whole-number sum in a comparison), and otherwise in 16.16 with the original's code and values.

#### Scenario: Quick and a small whole number without a Long
- **WHEN** a program executes `NPrint i*300` with `i=300` and `NPrint q+20000` with `q=20000`
- **THEN** it prints 24464 and -25536, the 16.16 results of the original

#### Scenario: Quick destination
- **WHEN** a program evaluates `t.q=w+v+q` with `w=3`, `v=4`, `q=1.5`, and `t.q=q*40000` with `q=1.5`
- **THEN** `t` is 8.5, and -5536 (60000 beyond the Quick range wrapped, as today)

#### Scenario: A Long operand under no wanted type
- **WHEN** a program prints `NPrint q*100000`, `NPrint Int(q)*100000` and `NPrint q+&l-&l` with `q=1.5`
- **THEN** it prints 150000, 100000 and 1.5 (today 1)

### Requirement: The result is rounded once, where it is used
The result of an expression with a Quick SHALL be rounded once, where it is used: down to a whole number, as the 16.16 value for a Quick, and without a wanted type (`If`, `Print`) as the Quick when it fits, else its whole part. A `/` under a wanted Long SHALL give a whole number towards zero at the operator; `+`, `-` and `*` SHALL keep the fraction until the result is used.

#### Scenario: Long destination
- **WHEN** a program evaluates `r.l=q*2`, `r.l=l-q`, `r.l=l*q`, `r.l=q/h`, `r.l=l+q`, `r.l=q+p` with `q=1.5`, `h=0.5`, `p=2.75`, `l=10`
- **THEN** it gets 3, 8, 15, 3, 11 and 4, as today

#### Scenario: Division under a Long destination
- **WHEN** a program evaluates `r.l=i/2*2` with `i=7` and `r.l=q/2+0` with `q=-3`
- **THEN** it gets 6 and -1 (the quotient is whole at the operator, as in the original)

#### Scenario: Word destination
- **WHEN** a program evaluates `x.w=q/2` with `q=-3` and `x.w=l*q` with `l.l=100000`, `q=0.25`
- **THEN** it gets -2 (the 16.16 quotient rounded down) and 25000 (the Long takes part: 48.16)

### Requirement: A constant with a point is fractional
A constant written with a point (`3.0`, `1.5`) SHALL be a fractional number whatever its value, also under a Byte type; a constant without a point SHALL be integral and take its smallest whole-number type (Byte, Word, Long). A fractional constant SHALL be a Float, as in the original, where it stands beside whole numbers only or a Long is wanted, and a Quick where a Quick is in its expression or wanted (and the value fits).

#### Scenario: Beside whole numbers or under a wanted Long a Float
- **WHEN** a program evaluates `r.l=l*0.1` with `l=100`, `NPrint l*0.001` with `l=1000000`, `r.l=q*1.1` with `q=10` and `r.l=l*0.5` with `l=-3`
- **THEN** it gets 10, 1000, 11 and -1, as the original

#### Scenario: Under a wanted Quick a Quick
- **WHEN** a program evaluates `x.q=l*1.1` with `l.l=10`
- **THEN** x is $000AFFFA, as the original

#### Scenario: The point asks for the fractional division
- **WHEN** a program evaluates `r.l=(l1/3.0>3.2)`, `rf.f=(l1/3.0>3.2)`, `If l1/3.0>3.2`, `rf.f=l1/3.0` and `rq.q=l1/3.0` with `l1.l=10`
- **THEN** it gets -1, -1, a true condition, 3.333333 and $00035555

#### Scenario: Under a Byte type
- **WHEN** a program evaluates `rb.b=b/2.0` and `rb.b=vb+2.0` with `b.b=3`, `vb.b=3`
- **THEN** it gets 1 and 5, computed in Float (today the whole Byte 2)

#### Scenario: A comparison with a fraction constant
- **WHEN** a program evaluates `If q=0.1` with `q=0.1`
- **THEN** the condition is true (two Quicks are compared; today false, the constant a Float)

### Requirement: A whole-number subexpression takes the fractional format of its expression
In an expression with a Quick or Float that is not wanted as a Long, a whole-number subexpression SHALL be computed in the expression's fractional format. A comparison SHALL compute both operands in one common format, the wider class of its two sides (whole numbers at least Long, Quick with whole numbers 48.16, Float), which reaches into both operands.

#### Scenario: Whole-number division beside a Quick
- **WHEN** a program executes `NPrint q+w/2` and `NPrint w/2+q` with `q=0.5`, `w=3`
- **THEN** both print 2 (today 2 and 1.5)

#### Scenario: Comparison with a fraction constant
- **WHEN** a program evaluates `rf.f=(l1/3>3.2)`, `rq.q=(l1/3>3.2)`, `r.l=(l1/3>3.2)` and `If l1/3>3.2` with `l1.l=10`
- **THEN** it gets -1, -1, -1 and a true condition (today -1, -1, 0 and false)

#### Scenario: Conditions that change
- **WHEN** a program evaluates `If w/2>q` with `w=3`, `q=1.2`, `If w/2*2<>x` with `x=3` and `If w/2*2=q` with `q=2`
- **THEN** the first is true and the other two false (today false, true, true); `If w/2*2=w` stays false

### Requirement: Mod keeps today's sign rule, exact with fractions
`Mod` SHALL give the remainder by today's rule (a negative remainder has the divisor added, as `domodlong`), computed with the fractions of its operands.

#### Scenario: Negative dividend
- **WHEN** a program evaluates `x.l=q Mod 2` with `q=-7`
- **THEN** x is 1, as today

#### Scenario: Fraction
- **WHEN** a program evaluates `x.q=q Mod 2` with `q=7.5`
- **THEN** x is 1.5 (today 1)

### Requirement: A Quick used as a whole number keeps working
Booleans in Quicks, bit operators, shifts and `Not` on Quicks SHALL give today's values: the operands of bit operators, shifts and `Not` are typed by the same rule as any operand and rounded down once where the operator takes them; a comparison's boolean in arithmetic is a whole number (Word -1 or 0), and with a Long the operation is a Long one.

#### Scenario: Flags and masks
- **WHEN** a program evaluates `h=Not h` with `h=-1`, `If (q AND 1)=1` with `q=3` and with `q=1.5`, `x.w=(flag=-1)*5` with `flag=-1`, `r.l=x-(a>b)` with `x=10`, `a=2`, `b=1`, and `If q Mod 2=1` with `q=7`
- **THEN** it gets 0, true, true, -5, 11 and true, as today

#### Scenario: Shifts of a Quick under a Long
- **WHEN** a program evaluates `r.l=q LSL 2` and `r.l=q ASL 4` with `q=1.5`
- **THEN** it gets 4 and 16, as today (the Quick rounded down before the shift)

#### Scenario: A boolean times a Long
- **WHEN** a program executes `NPrint l*(a>b)` and `NPrint (a>b)*100000` with `l.l=100000`, `a=2`, `b=1`
- **THEN** both print -100000

#### Scenario: A Long and a Quick under AND in both orders
- **WHEN** a program evaluates `If q AND l` and `If l AND q` with `l.l=65536`, `q=-1`
- **THEN** both conditions are true (today the first is false)

### Requirement: 16.16 operations give today's values
Where the rule computes in 16.16, the compiler SHALL give today's values bit for bit: the products of `qmul`, the quotients of `qdivide` with its last bit (up to 1/65536 above the quotient towards zero), and for a divisor 0 the largest number with the dividend's sign ($7FFFFFFF, for a negative dividend $80000001); a 48.16 division by 0 SHALL give the largest value with the dividend's sign, as `lqdiv` ($7FFFFFFF or -$7FFFFFFF as its whole part).

#### Scenario: qdivide's last bit
- **WHEN** a program evaluates `x.q=q/p` with `q=-7.25`, `p=0.3`
- **THEN** x is $FFE7D508, as today

#### Scenario: Division by zero
- **WHEN** a program evaluates `x.q=q/p` with `p=0` and `q=1`, and with `q=-1`, and `r.l=l/q` with `l.l=-100`, `q=0`
- **THEN** x is $7FFFFFFF and $80000001, r is -2147483647, as today

#### Scenario: 48.16 division by zero without a wanted Long
- **WHEN** a program executes `NPrint l/q` with `l.l=-100`, `q=0`
- **THEN** it prints -2147483647, as today

## MODIFIED Requirements

### Requirement: A Quick operand is computed with its fraction
When `+`, `-`, `*` or `/` has a Quick operand and the other operand or the wanted type is a Byte, Word or Long, the compiler SHALL compute it with the Quick's fraction in the format of "The wanted type and a Long set the format of the intermediate results" and give a whole number where it is used: down for `+`, `-`, `*`, towards zero for `/` under a wanted Long. This SHALL hold also for two Quicks under a wanted whole number. The Quick SHALL NOT be cut when it is fetched.

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

### Requirement: A Long operand that sets the type itself
When a Long operand takes part in an operation with a Quick (a constant beyond the Quick range, `&var`, `?label`, a Long variable), the operation SHALL be computed in 48.16 with the Quick's fraction ("The wanted type and a Long set the format of the intermediate results"), for `+` and `-` as for `*` and `/`. `Int(q)` gives the whole-number result.

#### Scenario: Multiplication by a large constant
- **WHEN** a program prints `NPrint q*100000` and `NPrint Int(q)*100000` with `q=1.5`
- **THEN** it prints 150000 and 100000

#### Scenario: Address operand
- **WHEN** a program prints `NPrint q+&l-&l` with `q=1.5`
- **THEN** it prints 1.5 (before: 1, the Quick rounded down for `+` and `-`)

#### Scenario: Quick destination
- **WHEN** a program evaluates `t.q=q*40000` with `q=1.5`
- **THEN** `t` is -5536, 60000 beyond the Quick range wrapped (the original: -25536)

### Requirement: Comparisons that were right keep their results
A comparison of whole numbers, of two Quicks, of a Float with anything, and of strings SHALL give the same result as before, including the byte-constant rule (an equality comparison of a Byte with a constant from 128 to 255). A comparison whose operands hold whole numbers in a Quick SHALL give the same boolean as before, as long as no intermediate value overflows.

#### Scenario: Sources unchanged
- **WHEN** the corpus and the CD's sources that compile are compiled and run before and after
- **THEN** the outputs are the same except for the places the change lists; every executable that differs is named for the comparison with the previous commit

#### Scenario: Compound right operand in its own types (accepted until #1180)
- **WHEN** a program evaluates `If base<i*4+base` with `base.l=$40000` and a default-typed (Quick) `i=20000`
- **THEN** with the operand typing (#1180) the condition is true: the Long takes part, so `i*4+base` is computed in 48.16 (before the change the Quick product overflowed and the condition was false)

### Requirement: No other result changes
A program that has no Quick operand in an arithmetic expression under a whole-number type so far, and a program whose Quick operands in such expressions hold whole numbers, SHALL give the same results as before the change, except where an intermediate value overflowed before and is now computed in a wider format, and except the cases the requirements of this change name. Array indexes (#1179) are not changed by this requirement.

#### Scenario: Sources unchanged
- **WHEN** the corpus and the CD's sources that compile are compiled and run before and after
- **THEN** the outputs are the same except for the places the change lists (the named matrix cells, the CD places of the prev run); every other executable that differs is named for the comparison with the previous commit

## REMOVED Requirements

### Requirement: Known limits of the left operand until the expression tree
**Reason**: the limit (#1183) ends with the operand typing: the left operand of a comparison is typed by its own operands (requirement "The left operand of a comparison is compared with its own value whatever its shape").
**Migration**: none; `-q` on the left keeps its scenario in the new requirement.

### Requirement: The expected type flows as before
**Reason**: replaced by the operand typing ("The type and format of an operation do not depend on the order of its operands", "The wanted type and a Long set the format of the intermediate results"); its scenarios move there (`NPrint 1+1+1+q` now 4.5, `NPrint w+v+q` 8.5).
**Migration**: none; `adr.l=base+i*4` gives `base+80000` in both orders.

### Requirement: Rounding happens once per operation
**Reason**: a chain under a wanted whole number is now computed in its format and rounded once where it is used (`r.l=l+q+p` 14, before 13); the Quick destination and the bit operators keep their values (requirements "The result is rounded once, where it is used", "A Quick used as a whole number keeps working", "NOT, AND, OR and EOR under a Quick type take the whole part").
**Migration**: none.

### Requirement: A whole-valued literal stays whole under a Byte type
**Reason**: a constant written with a point is fractional also under a Byte type (user, 2026-11-24, F6: "Neue Regel überall"); requirement "A constant with a point is fractional". `rb.b=vb+2.0` keeps its value 5.
**Migration**: the case `rb.b=vb AND 2.0` (compiles today) is an open point of the change (design, "Open questions").
