# Delta: compiler/expression-types

## ADDED Requirements

### Requirement: The destination does not change the computation
The compiler SHALL type and compute every operation from its operator and its operands only. A destination, a parameter type or an index SHALL only convert the finished value, except for the named exceptions: the shift exception and its left-shift wrap, the whole-division exception and the top operation under a Float destination (their requirements below).

#### Scenario: The same quotient without an exception
- **WHEN** a program evaluates `z.l=(7/2)*2`, `r.w=7/2` and executes `NPrint 7/2*2`
- **THEN** z is 6, r is 3 and it prints 6

#### Scenario: A whole-number Quick under a Long and without a destination
- **WHEN** a program with a default-typed `i=300` evaluates `r.l=i*300` and executes `NPrint i*300`
- **THEN** r is 90000 and it prints 90000 (before: 90000 and 24464)

### Requirement: Only the named exceptions depend on the destination
The exceptions named in the requirement "The destination does not change the computation" SHALL be the only ways in which a destination changes the computation inside an expression. A Quick or Float parameter SHALL take them like a destination of its type. Each place where one of them gives another type or conversion than the other rules SHALL give its warning (compiler/warnings).

#### Scenario: An exception and no exception
- **WHEN** a program evaluates `x.q=w/9` and `r.l=w/9` with `w.w=200`
- **THEN** x is 22.2222 with the warning `divfrac`, and r is 22 without a warning

#### Scenario: Typed parameters
- **WHEN** a program with `Statement pq{x.q}` and `Statement pf{x.f}` calls `pq{1/16}`, `pq{1 LSL 16}` and `pf{q/100}` with `q=20.875`
- **THEN** pq gets 0.0625 and 0, and pf gets 0.20875, as the original

### Requirement: A whole-number operation is one step wider than its operands
An operation on two whole operands SHALL be computed one step wider than the wider of them: Byte to Word, Word to Long, Long stays Long; the unary minus and `NOT` of a whole operand SHALL be one step wider than the operand, and `NOT` of a Quick a Long. Its value SHALL therefore be the exact value for `+`, `-` and `*` up to a Word operand, and the exact value modulo 2^32 for a Long.

#### Scenario: Byte and Word sums without a narrow destination
- **WHEN** a program executes `NPrint b+c` with `b.b=100`, `c.b=100` and `NPrint w+v` with `w.w=20000`, `v.w=20000`
- **THEN** it prints 200 and 40000 (before: 200 and -25536)

#### Scenario: A narrow destination wraps the exact value
- **WHEN** a program evaluates `x.b=b+c` and `y.w=w+v` with the same values
- **THEN** x is -56 and y is -25536

#### Scenario: Unary minus is one step wider
- **WHEN** a program executes `NPrint -q` and evaluates `x.q=-q` with `q=-32768`
- **THEN** it prints 32768 and x is $7FFFFFFF (before: -32768 and -32768)

#### Scenario: NOT is one step wider
- **WHEN** a program executes `NPrint (Not b) LSR 1` with `b.b=0`, `NPrint (Not w) LSR 1` with `w.w=0` and evaluates `x.w=(Not w) LSR 1`
- **THEN** it prints 32767 (before: 2147483647) and 2147483647, and x is -1

#### Scenario: A Long result wraps
- **WHEN** a program evaluates `r.l=l*l` with `l.l=100000`
- **THEN** r is 1410065408

### Requirement: An operation with a Quick is computed in 48.16
An operation with a Quick operand and a whole or Quick operand SHALL convert both to 48.16 and compute the exact result there; a product or quotient SHALL be cut down to 1/65536. A 48.16 value SHALL wrap beyond 48 integer bits.

#### Scenario: Quick sums and products beyond the Quick range
- **WHEN** a program executes `NPrint q+20000` with `q=20000` and evaluates `x.q=q*p*r` and `y.q=r*q*p` with `q=300`, `p=300`, `r=0.25`
- **THEN** it prints 40000, and x and y are both 22500 (before: -25536, 6116 and 22500)

#### Scenario: The cut of a product under a Long
- **WHEN** a program evaluates `r.l=q*p` with `q=-0.5`, `p=1/65536`
- **THEN** r is -1

### Requirement: An operation with a Float is computed in Float
An operation with a Float operand SHALL convert the other operand to Float (a Quick or 48.16 value with its fraction) and compute in Float; `^` SHALL always compute in Float.

#### Scenario: A Quick and a Float
- **WHEN** a program evaluates `r.l=q+f` with `q=1.5`, `f.f=0.5`
- **THEN** r is 2

#### Scenario: A Quick quotient below the top before a Float
- **WHEN** a program evaluates `x.f=q/100+f` with `q=20.875`, `f.f=0`
- **THEN** x is 0.2087402 (the 48.16 quotient; before: 0.20875)

### Requirement: A whole constant takes its smallest type
A constant written without a point SHALL be a Byte from -128 to 127, a Word from -32768 to 32767, else a Long; `DEFTYPE` SHALL NOT change it. A `$` or `%` constant SHALL be whole. A minus before a constant SHALL be the unary minus, one step wider (`-8` a Word).

#### Scenario: Constants in a sum
- **WHEN** a program executes `NPrint 100+100` and `NPrint 20000+20000`
- **THEN** it prints 200 and 40000

#### Scenario: A negated constant
- **WHEN** a program executes `NPrint -8 LSR 1`
- **THEN** it prints 32764 (the original: 2147483644)

#### Scenario: DEFTYPE has no influence
- **WHEN** a program with `DEFTYPE .f` evaluates `x.l=7/2`
- **THEN** x is 3

### Requirement: A constant with a point is read by its operands
A constant with a point or an exponent SHALL never be whole. It SHALL be a Quick when its value is exact in 16.16 or when a Quick is the other operand of the same operator, also with a unary minus before the constant; else the Float of the written value. Read as a Quick, it SHALL be the FFP value of its text cut down to 1/65536, as a Quick literal.

#### Scenario: A point and no fraction
- **WHEN** a program executes `NPrint 3/2.0` and `NPrint 7/2.0+0.5`
- **THEN** it prints 1.5 and 4 (before: 1 and 3.5; the original: 1 and 3; #1231)

#### Scenario: Beside a Quick
- **WHEN** a program evaluates `If q=0.1` with `q=0.1` and `x.q=(0.1=0.1)`, `y.q=(1.3<1.3)`
- **THEN** the condition is true, x is -1 and y is 0 (#1230)

#### Scenario: Under a unary minus and beyond the exact value
- **WHEN** a program evaluates `x.q=q+(-0.1)` with `q=3`, `y.q=q2+30000.3` with `q2=1` and executes `NPrint q3*(-0.3)` with `q3=157`
- **THEN** x is $0002E667, y is $75314D00 and it prints -47.0981, as the original

#### Scenario: Beside whole numbers
- **WHEN** a program evaluates `r.l=l*0.5` with `l.l=-3`, `r2.l=l2*0.1` with `l2.l=100` and executes `NPrint l3*0.001` with `l3.l=1000000`
- **THEN** r is -2 (0.5 is exact: a Quick; before: -1), r2 is 10 and it prints 1000 (0.1 and 0.001 are not exact: Floats)

#### Scenario: Beside a Quick variable
- **WHEN** a program evaluates `x.q=q+0.1` with `q=1`
- **THEN** x is $00011999

### Requirement: A whole division rounds towards zero
`/` of two whole operands SHALL give a whole number, rounded towards zero, whatever the destination.

#### Scenario: Negative and positive
- **WHEN** a program evaluates `r.l=-7/2`, `r2.l=i/2*2` with `i.l=7`, `x.q=(7/2=3)` and executes `NPrint 7/2`
- **THEN** r is -3, r2 is 6, x is -1 and it prints 3

#### Scenario: A Quick operand keeps the fraction
- **WHEN** a program evaluates `r.l=q/2*2` with `q=7` and `x.q=7.0/2`
- **THEN** r is 7 and x is 3.5

### Requirement: The whole-division exception under a Quick or Float destination
Under a Quick or Float destination, a `/` of two whole operands that is the whole expression or is reached from it only through `+`, `-`, `*`, `/` and the unary minus, at any depth, SHALL keep its fraction, for constants and variables alike: under a Quick a 48.16 quotient cut down, under a Float a quotient computed directly in Float. A whole `/` below any other operator, and under any other destination, SHALL stay whole.

#### Scenario: The top division
- **WHEN** a program evaluates `x.q=1/16`, `y.q=w/9` with `w.w=200` and `z.f=7/2`
- **THEN** x is 0.0625, y is 22.2222 ($001638E3) and z is 3.5

#### Scenario: Through + - *
- **WHEN** a program evaluates `x.q=15-(w/16)` with `w.w=117`, `y.q=1-(v/15)` with `v.w=70`, `z.f=(7/2)*2` and `u.q=2*(4/200)`
- **THEN** x is 7.6875, y is $FFFC5556 (-3.6667, as the original), z is 7 and u is $00000A3C (the original: $00000A3D, its quotient cut towards zero)

#### Scenario: Through a division and a unary minus
- **WHEN** a program with `a.w=7`, `b.w=2`, `c.w=2`, `d.w=1` evaluates `x.q=(a/b)/c`, `y.q=a/(d/b)`, `z.q=-(a/b)`, `fx.f=(a/b)/c`, `fy.f=a/(d/b)`, `fz.f=-(a/b)` and `r.l=(a/b)/c`
- **THEN** x and fx are 1.75, y and fy are 14, z and fz are -3.5, as the original, and r is 1

#### Scenario: Below another operator or another destination
- **WHEN** a program evaluates `x.q=(7/2=3)`, `y.q=(7/2) AND 3` and `z.l=(7/2)*2`
- **THEN** x is -1, y is 3 and z is 6

### Requirement: A whole division beside a Quick keeps its fraction
A `/` of two whole operands whose other operand at an arithmetic operator (`+ - * /`, `Mod`, `^`) or a comparison is of Quick class (a Quick variable, a subexpression with a Quick result, a constant with a point that is a Quick; not a Float) SHALL keep its fraction as a 48.16 quotient, under every destination, also when a unary minus stands between the `/` and that operator; at a bit operator or a shift it SHALL stay whole.

#### Scenario: Beside a Quick
- **WHEN** a program executes `NPrint q+w/2` and `NPrint w/2+q` with `q=0.5`, `w.w=3`, and evaluates `x.w=(q2+(w2/2))-2` with `q2=149.75`, `w2.w=135`
- **THEN** both print 2 and x is 215, as the original

#### Scenario: Other Quick-class neighbours and a Float
- **WHEN** a program with `q=0.25`, `n.w=2`, `w.w=3`, `d.w=2`, `f.f=0.5` executes `NPrint q+w/d`, `NPrint 0.5+w/2`, `NPrint (q*n)+w/2` and `NPrint f+w/2`
- **THEN** it prints 1.75, 2, 2 and 1.5

#### Scenario: Other operators
- **WHEN** a program evaluates `x.l=q AND w/2` with `q=5`, `w.w=-3` and executes `NPrint q2<w2/2` with `q2=1`, `w2.w=3`
- **THEN** x is 5 and it prints -1, as the original

#### Scenario: Through a unary minus
- **WHEN** a program executes `NPrint q+-(w/2)` with `q=0.5`, `w.w=3`
- **THEN** it prints -1

### Requirement: A whole division reached by two exceptions under a Float destination
Where the whole-division exception and a Quick-class neighbour both reach a whole `/` under a Float destination, its quotient SHALL be the Float quotient of the whole-division exception.

#### Scenario: Both exceptions
- **WHEN** a program evaluates `y.f=q+w/3` with `q=0.5`, `w.w=1`
- **THEN** y is 0.8333334, as the original

### Requirement: The top operation under a Float destination runs in Float
Under a Float destination the arithmetic operation (`+ - * /`, `Mod`, `^`) that is the whole expression SHALL be computed in Float when one of its operands is a Quick; its operands are converted to Float first, and a constant with a point there is read directly as the Float of its written value, not first as a Quick.

#### Scenario: Quick operands at the top
- **WHEN** a program evaluates `x.f=q/100` with `q=20.875`, `y.f=q2*0.002` with `q2=92` and `z.f=q3/q4` with `q3=17.5`, `q4=113.5`
- **THEN** x is 0.20875, y is 0.184 and z is 0.1541850, as the original

#### Scenario: A comparison or a unary minus at the top
- **WHEN** a program evaluates `x.f=(q=0.1)` with `q=0.1` and `y.f=-(q2/100)` with `q2=20.875`
- **THEN** x is -1 (1.95: 0) and y is -0.2087402 (1.95: -0.20875)

### Requirement: The conversion to a whole destination rounds down and wraps
The conversion of a Quick or 48.16 value to a Byte, Word or Long (an assignment, a parameter, an index) SHALL round down, and of a Float SHALL cut towards zero (as the original's `SPFix`); the result SHALL keep the low 8, 16 or 32 bits. A Float beyond the Long range SHALL be cut towards zero and wrap to its low 32 bits, not saturate as `SPFix`. A `Data.b` value with a point SHALL be cut towards zero as a Float.

#### Scenario: Negative fractions
- **WHEN** a program evaluates `r.l=-1.5`, `r1.l=-1.4`, `r2.l=f` with `f.f=-2.5`, `w2.w=q` with `q=-0.5` and `r3.l=v^n` with `v.f=-2.5`, `n.w=3`
- **THEN** r is -2 (an exact constant: a Quick; before: -1), r1 is -1 (a Float), r2 is -2, w2 is -1 and r3 is -15

#### Scenario: A quotient at the top
- **WHEN** a program evaluates `r.l=q/2`, `x.w=q/2` with `q=-3` and `r2.l=k/q2` with `k.l=-10`, `q2=1.5`
- **THEN** r is -2, x is -2 and r2 is -7 (before: -1, -1 and -6)

#### Scenario: Beyond the range
- **WHEN** a program evaluates `x.w=l` with `l.l=100000` and `r.l=f` with `f.f=1e10`
- **THEN** x is -31072 and r is 1410065408

#### Scenario: Data.b with a point
- **WHEN** a program reads `Data.b -1.5,1.0` into Byte variables
- **THEN** it reads -1 and 1

### Requirement: The conversion to a Quick rounds down and saturates
The conversion of a whole, 48.16 or Float value to a Quick SHALL round down to 1/65536 and SHALL give $7FFFFFFF above the Quick range and $80000000 below it.

#### Scenario: Saturation
- **WHEN** a program evaluates `x.q=b+100000` with `b.b=3`, `y.q=q*40000` with `q=1.5` and `z.q=l` with `l.l=100000`
- **THEN** x, y and z are $7FFFFFFF (before: $86A30000, $EA600000 and $86A00000)

#### Scenario: A Float rounded down
- **WHEN** a program evaluates `x.q=f/10` with `f.f=-144`
- **THEN** x is $FFF19999 (before: $FFF1999A, towards zero)

### Requirement: Bit operators take whole operands
`AND`, `OR`, `EOR`, `NOT`, `&`, `|` and the bit test operators SHALL round a Quick or 48.16 operand down to a whole number and give a whole result. A Float operand SHALL stay refused with "Illegal Operator for Type". Under a Float destination, whole operands SHALL compile: the whole result converts to Float at the end.

#### Scenario: Quick operands
- **WHEN** a program executes `NPrint q AND 3` with `q=7.5` and evaluates `h=Not h` with a default-typed `h=-1`
- **THEN** it prints 3 and h is 0

#### Scenario: A Float operand
- **WHEN** a program contains `r.l=l AND f` with `f.f=1.5`
- **THEN** compiling it gives "Illegal Operator for Type"

#### Scenario: Whole operands under a Float destination
- **WHEN** a program evaluates `rf.f=b AND 1` with `b.b=3`, `rf.f=Not w` with `w.w=5` and `rf.f=l AND f` with `l.l=5`, `f.f=1.5`
- **THEN** rf is 1.0 and -6.0 (before and in the original: "Illegal Operator for Type"), and the third gives "Illegal Operator for Type"

### Requirement: A shift keeps the type of its left operand
A Quick left operand SHALL keep its fraction bits: `LSL`, `ASL`, `ASR` in 48.16, `LSR` on the 16.16 bits. A whole left operand SHALL be shifted as a whole number: `LSR`, `ASR` in its own width, `LSL`, `ASL` as a Long. The count SHALL be whole (a Quick rounded down, a constant with a point refused) and SHALL NOT make the left operand fractional.

#### Scenario: A Quick under a Long
- **WHEN** a program evaluates `r.l=q LSL 2` with `q=1.5` and executes `NPrint q LSR 1` with `q=3`
- **THEN** r is 6 (before: 4) and it prints 1.5

#### Scenario: A whole shift beside a Quick
- **WHEN** a program executes `NPrint qa+3 LSR 3` and `NPrint qa+ba LSR bb` with `qa=1.25`, `ba.b=3`, `bb.b=3`, and evaluates `x.l=qa+3 LSR 3`
- **THEN** both print 1.25 and x is 1 (before: 1.625, 1.25 and 1)

#### Scenario: A Quick count
- **WHEN** a program executes `NPrint x LSR h` with `x.w=3`, `h=2.5`
- **THEN** it prints 0 (before and in the original: .75)

#### Scenario: The widths of whole shifts
- **WHEN** a program evaluates `x.w=w LSR 3` with `w.w=-3`, executes `NPrint b LSR 1` with `b.b=-8`, `NPrint w2 LSL 4` with `w2.w=$1000` and evaluates `r.l=1 LSL 17`
- **THEN** x is 8191, it prints 124 and 65536, and r is 131072

#### Scenario: A Quick shifted in 48.16
- **WHEN** a program executes `NPrint q LSL 1` with `q=20000`
- **THEN** it prints 40000

### Requirement: A shift at the top under a Quick destination
Under a Quick destination, a shift of a whole left operand that is the whole expression or is reached from it through binary `+`, `-` only (not through a unary minus) SHALL be the shift exception: a right shift (`LSR`, `ASR`) shifts the value with 16 fraction bits (on the 16.16 bits inside the Quick range, in 48.16 beyond it); a left shift (`LSL`, `ASL`) shifts it as a Long.

#### Scenario: Right shifts
- **WHEN** a program evaluates `x.q=1 LSR 7`, `y.q=b LSR 3` with `b.b=3` and `z.q=1 LSR 4`
- **THEN** x is 1/128 ($00000200), y is 0.375 and z is 0.0625

#### Scenario: Beyond the Quick range and negative values
- **WHEN** a program evaluates `x.q=100000 LSR 2` and `y.q=vb LSR 3` with `vb.b=-3`
- **THEN** x is 25000 and y is $1FFFA000

#### Scenario: Through + -
- **WHEN** a program evaluates `x.q=qa+3 LSR 3` with `qa=1.25` and `y.q=(v LSR 3)+14` with `v.b=63`
- **THEN** x is 1.625 and y is 21.875, as the original

#### Scenario: Below another operator
- **WHEN** a program evaluates `x.q=qa*(3 LSR 3)` with `qa=1.25` and `y.q=-(1 LSR 7)`
- **THEN** x and y are 0 (1.95: y is -1/128)

### Requirement: A left shift at the top under a Quick destination wraps
Under a Quick destination, a left shift (`LSL`, `ASL`) that is the whole expression, of a whole or of a Quick left operand, SHALL convert to the Quick wrapped (the low 32 bits of its 16.16 form), not saturated. A left shift reached through `+`, `-` SHALL be a Long, and the sum SHALL convert saturated as any other value.

#### Scenario: Left shifts
- **WHEN** a program evaluates `x.q=1 LSL 4` and `y.q=1 LSL 16`
- **THEN** x is 16 and y is 0 (the Long 65536 wrapped)

#### Scenario: A Quick left operand and a left shift in a sum
- **WHEN** a program evaluates `z.q=q LSL 2` with `q=10000` and `u.q=(v LSL 4)+p` with `v.w=4096`, `p=0.5`
- **THEN** z is $9C400000 (wrapped) and u is $7FFFFFFF (saturated)

### Requirement: Without a destination a 48.16 value is a Quick, a Long or a Float
Where no type is wanted (Print, NPrint, Str$ and untyped parameters), a whole result SHALL be passed as a whole number, and a 48.16 result SHALL be passed at run time as a Quick when it fits the Quick range, else as a Long when it is whole and fits a Long, else as a Float.

#### Scenario: The three forms
- **WHEN** a program executes `NPrint q+w` with `q=1.5`, `w.w=3`, `NPrint q*l` and `NPrint q+l` with `l.l=100000`
- **THEN** it prints 4.5, 150000 and 100001.5 (before: 4.5, 150000 and 100001)

### Requirement: Comparisons compare the full value
A comparison, `If`, `While`, `Until` and `Select`/`Case` SHALL compare the exact values of both operands (whole, 48.16 or Float, the wider of the two) without converting them first; the result of a comparison SHALL be a Word, -1 or 0, converted at the destination.

#### Scenario: A Quick sum beyond the Quick range
- **WHEN** a program evaluates `If q+q>40000` with `q=20000.5`
- **THEN** the condition is true

#### Scenario: A whole quotient compared with a fraction
- **WHEN** a program evaluates `r.l=(l/3>3.2)` and `r2.l=(l/3.0>3.2)` with `l.l=10`
- **THEN** r is 0 (before: -1) and r2 is -1

### Requirement: A constant expression gives the value of its variable form
Folding SHALL give for every operator, operand order and destination the value of the same expression with each constant replaced by a variable of the constant's type (a whole constant its smallest type, a constant with a point by the requirement "A constant with a point is read by its operands").

#### Scenario: Division (#1225)
- **WHEN** a program evaluates `x.l=1-1.25/3` and `y.l=1-q/3` with `q=1.25`
- **THEN** x and y are both 0

#### Scenario: Mod over a division (#1229)
- **WHEN** a program evaluates `x.l=(7/-3) Mod 3` and `y.l=100 Mod (7/3)`
- **THEN** x is 1 and y is 0, as with variables

#### Scenario: Shifts (#1233, the constant shift beside a Quick)
- **WHEN** a program executes `NPrint (-8 ASR 40)+qh` and `NPrint (bn ASR b40)+qh` with `qh=0.5`, `bn.b=-8`, `b40.b=40`
- **THEN** both print -.5

### Requirement: Mod takes the sign of the divisor
`Mod` SHALL give `a-b*floor(a/b)`: the remainder has the divisor's sign, computed exactly with the fractions of its operands, also for two constants. A divisor 0 SHALL raise the division trap (requirement "A division or Mod by a run-time 0 raises the division trap").

#### Scenario: Signs
- **WHEN** a program evaluates `r1.l=-7 Mod 3`, `r2.l=7 Mod -3`, `r3.l=-7 Mod -3` and `r4.l=q Mod 2` with `q=-7`
- **THEN** r1 is 2, r2 is -2, r3 is -1 (before: -4, #1197) and r4 is 1

#### Scenario: Fractions
- **WHEN** a program evaluates `x.q=q Mod 2` with `q=7.5`, `y.q=q Mod p` with `p=0.5` and `z.q=b Mod 1.5` with `b.b=4`
- **THEN** x is 1.5, y is 0 and z is 1

#### Scenario: Two constants
- **WHEN** a program evaluates `x.q=5.25 Mod 2`, `y.w=5.25 Mod 2`, `z.q=-5.25 Mod 2` and `r.l=-5.25 Mod -2`
- **THEN** x is 1.25, y is 1, z is 0.75 and r is -2 (before: -4)

#### Scenario: Divisor 0
- **WHEN** a program evaluates `r.l=l Mod z` with `l.l=-7`, `z.l=0`, and `x.w=w Mod z` with `w.w=7`, `z.w=0`
- **THEN** both raise the division trap (before: r is -7)

### Requirement: A division or Mod by a run-time 0 raises the division trap
A `/` or `Mod` whose divisor is 0 at run time SHALL raise the CPU's division trap for every type (Byte, Word, Long, Quick, 48.16 and Float), under every destination, also for a quotient whose fraction an exception keeps; with the runtime error checking on, the checks' "Division by Zero" error SHALL stay where it is given today. A constant divisor 0 SHALL stay the compile error "Division by Zero".

#### Scenario: Every type
- **WHEN** a program evaluates `x.l=w/z` with `w.w=7`, `z.w=0`, `y.q=l/q` with `l.l=100`, `q=0`, `r.f=f/fz` with `f.f=1`, `fz.f=0` or `u.q=a/z` with `a.w=7`
- **THEN** each raises the division trap (before: 2147483647, $7FFFFFFF, the trap, $7FFFFFFF)

#### Scenario: A constant divisor 0
- **WHEN** a program contains `x.l=7/0`
- **THEN** compiling it gives "Division by Zero", as before

### Requirement: Optimizations keep the value
The compiler SHALL use narrower code or another order of operations only where the result is the value of these rules for every operand value.

#### Scenario: A Byte sum under a Byte destination
- **WHEN** a program evaluates `x.b=b1+b2+b3` and executes `NPrint b1+b2+b3` with `b1.b=100`, `b2.b=100`, `b3.b=100`
- **THEN** x is 44 and it prints 300

### Requirement: Both compiler modes follow the rework's rules
blitz2 and blitz2-ab3 (AB3 mode) SHALL apply these rules alike; where AmiBlitz3 v3.14.0 differs, `docs/ab3/variant.md` SHALL have a row.

#### Scenario: AB3 mode
- **WHEN** an AB3 source evaluates `x.q=(7/2=3)` and `r.l=q LSL 2` with `q=1.5`
- **THEN** x is -1 and r is 6, as in blitz2

## MODIFIED Requirements

### Requirement: The type and format of an operation do not depend on the order of its operands
The compiler SHALL give each operation its type and its format (whole number, 48.16, Float) from its operator and its operands' types, never from the order of its operands, the place of a subexpression in a chain or the destination. The values in both orders SHALL agree unless a Long or 48.16 intermediate wraps. The rule SHALL be the same in AB3 mode.

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
- **THEN** both give 14

#### Scenario: Bit operator, shift and Not of a mixed chain
- **WHEN** a program evaluates `r.l=(l+q+p) AND 255` and `r.l=(q+p+l) AND 255`, `r.l=(l+q+p) LSL 1` and `r.l=(q+p+l) LSL 1`, `r.l=Not (l+q+p)` and `r.l=Not (q+p+l)` with `l=10`, `q=1.5`, `p=1.5`
- **THEN** they give 13 and 13, 26 and 26, -14 and -14

#### Scenario: A Float in the chain
- **WHEN** a program evaluates `r.l=l+q+f` and `r.l=f+q+l` with `l=10`, `q=1.5`, `f.f=0.5`
- **THEN** both give 12

#### Scenario: Overflow of a Word sum in both orders
- **WHEN** a program executes `NPrint w+v+q` and `NPrint q+w+v` with `w=20000`, `v=20000`, `q=0.5`
- **THEN** both print 40000.5 (before: -25535.5)

#### Scenario: The limit: an intermediate leaves its format
- **WHEN** a program evaluates `x.q=q*q2*r` and `x.q=r*q*q2` with `q=300`, `q2=300`, `r=0.25`
- **THEN** both give 22500 (before: 6116 and 22500); only a 48.16 intermediate beyond 48 bits wraps

#### Scenario: AB3 mode
- **WHEN** an AB3 source evaluates `r.l=(l+q+p) AND 255` and `r.l=(q+p+l) AND 255` with `l=10`, `q=1.5`, `p=1.5`
- **THEN** both give 13, as in blitz2

### Requirement: A Quick used as a whole number keeps working
Booleans in Quicks, bit operators and `Not` on Quicks SHALL give today's values: a Quick operand of a bit operator or `Not` is rounded down once where the operator takes it; a comparison's boolean in arithmetic is a whole number (-1 or 0). A shift of a Quick keeps its fraction bits (requirement "A shift keeps the type of its left operand").

#### Scenario: Flags and masks
- **WHEN** a program evaluates `h=Not h` with `h=-1`, `If (q AND 1)=1` with `q=3` and with `q=1.5`, `x.w=(flag=-1)*5` with `flag=-1`, `r.l=x-(a>b)` with `x=10`, `a=2`, `b=1`, and `If q Mod 2=1` with `q=7`
- **THEN** it gets 0, true, true, -5, 11 and true, as today

#### Scenario: Shifts of a Quick under a Long
- **WHEN** a program evaluates `r.l=q LSL 2` and `r.l=q ASL 4` with `q=1.5`
- **THEN** it gets 6 and 24 (before: 4 and 16, the Quick rounded down before the shift)

#### Scenario: A boolean times a Long
- **WHEN** a program executes `NPrint l*(a>b)` and `NPrint (a>b)*100000` with `l.l=100000`, `a=2`, `b=1`
- **THEN** both print -100000

#### Scenario: A 48.16 operand of a bit operator or shift
- **WHEN** a program evaluates `r.l=(q+40000) AND b` with `q=1.5`, `b.b=3`, and `r.l=(l+q) LSL 1` with `l.l=100000`, `q=1.5`
- **THEN** it gets 1 and 200003 (before: 200002; the shift in 48.16)

#### Scenario: A Long and a Quick under AND in both orders
- **WHEN** a program evaluates `If q AND l` and `If l AND q` with `l.l=65536`, `q=-1`
- **THEN** both conditions are true

### Requirement: A shift count is a whole number
The count of `LSL`, `ASL`, `LSR` and `ASR` SHALL be a constant without a point or a variable or expression of a Byte, Word, Long or Quick type; a Quick count SHALL be its whole part, rounded down. A constant with a point as a count SHALL be refused with "Illegal Operator for Type", whatever its value. A constant with a point as the shifted operand SHALL be a Quick of its value.

#### Scenario: A constant with a point as count
- **WHEN** a program contains `rb.b=b LSL 2.0`, `NPrint q LSL 2.5` or `x.w=x LSL 2.0`
- **THEN** compiling it gives "Illegal Operator for Type"

#### Scenario: A constant with a point as the shifted operand
- **WHEN** a program evaluates `rb.b=1.5 LSL 2`, `r.l=1.5 LSL 2`, `NPrint 1.5 LSR 1`, `NPrint 2.0 LSL 1` and `NPrint 1.5 LSL qh` with `qh=2.5`
- **THEN** it gets 6, 6 (before: 4), .75, 4 and 6

#### Scenario: Quick variables as count
- **WHEN** a program executes `NPrint x LSL q2`, `NPrint x LSL qh` and `NPrint x LSL qn` with `x.w=3`, `q2=2.0`, `qh=2.5`, `qn=2.99`
- **THEN** it prints 12, 12 and 12

#### Scenario: A Quick count beside a whole or Quick left operand
- **WHEN** a program executes `NPrint x LSR qh` with `x.w=3`, `qh=2.5` and evaluates `rl.l=q LSL qh` with `q=1.5`
- **THEN** it prints 0 (before: .75) and rl is 6 (before: 4)

### Requirement: Every array index is evaluated as a Long
The compiler SHALL evaluate every index of an array (the first and every later one) by the rules of this capability and convert it to a Long as a Long destination does (rounded down). An index SHALL address the same element with and without runtime error checking whenever the checked build does not report it.

#### Scenario: Quick calculation beyond the Quick range
- **WHEN** a program with `Dim g.b(1,60000),h.b(60000,1)` and a default-typed `y=200` reads `g(1,y*200/2)` and `h(y*200/2,1)`, compiled without and with runtime error checking
- **THEN** both builds read `g(1,20000)` and `h(20000,1)`

#### Scenario: Shift of a Quick in a later index
- **WHEN** a program reads `b(1,q ASL 4)` with `q=1.5`, compiled without runtime error checking
- **THEN** it reads `b(1,24)` (before: 16; the original: 24 without and 16 with runtime error checking)

#### Scenario: Quick with a fraction keeps its value
- **WHEN** a program reads `b(1,q*10)`, `c(1,5,q*10)` and assigns `v(1,q*10)=99` with `q=1.5`, compiled without and with runtime error checking
- **THEN** both builds use element 15 of the later index

#### Scenario: Large whole-number indexes
- **WHEN** a program with `Dim g.b(1,60000)` reads `g(1,40000)`, `g(1,l)` with `l.l=40000` and `g(1,wv*2)` with `wv.w=20000`, without and with runtime error checking
- **THEN** both builds read `g(1,40000)`, as before

## REMOVED Requirements

### Requirement: A 48.16 division by 0 gives the largest value of the wanted type
**Reason**: A division or Mod by a run-time 0 raises the division trap for every type, "type-system-rework D1/D7 (division and Mod by a run-time 0), D5" of 2026-11-28.
**Migration**: Requirement "A division or Mod by a run-time 0 raises the division trap".

### Requirement: Mod keeps today's sign rule, exact with fractions
**Reason**: Mod is floor Mod (the divisor's sign), "type-system-rework Q1, Q2, Q4, Q7" of 2026-11-27.
**Migration**: Requirement "Mod takes the sign of the divisor".

### Requirement: Comparisons that were right keep their results
**Reason**: The rework changes comparisons whose Quick operands overflowed 16.16 and whose whole quotients took a fraction from the other side.
**Migration**: Requirements "Comparisons compare the full value" and "The destination does not change the computation".

### Requirement: A Quick operand is computed with its fraction
**Reason**: Replaced by the operand-only rules; a `/` under a Long is no longer whole at the operator and the conversion rounds down.
**Migration**: Requirements "An operation with a Quick is computed in 48.16" and "The conversion to a whole destination rounds down and wraps".

### Requirement: A Long operand that sets the type itself
**Reason**: Every operation with a Quick is 48.16 now, with or without a Long; a Quick destination saturates.
**Migration**: Requirements "An operation with a Quick is computed in 48.16" and "The conversion to a Quick rounds down and saturates".

### Requirement: Other operators take the Quick rounded down
**Reason**: A shift keeps the Quick's fraction bits under every destination; Mod is exact.
**Migration**: Requirements "Bit operators take whole operands", "A shift keeps the type of its left operand" and "Mod takes the sign of the divisor".

### Requirement: No other result changes
**Reason**: The rework changes results by design; the changed results are named in the change and in the prev comparison.
**Migration**: design.md of type-system-rework, "Measurements".

### Requirement: A fraction literal keeps its fraction under a Byte type
**Reason**: A constant with a point is a Quick by default under every destination, and the conversion rounds down (`rb.b=-1.5` -2).
**Migration**: Requirement "A constant with a point is read by its operands".

### Requirement: The wanted type and a Long set the format of the intermediate results
**Reason**: The destination does not change the computation; there is no 16.16 operation any more.
**Migration**: Requirements "The destination does not change the computation" and "An operation with a Quick is computed in 48.16".

### Requirement: The result is rounded once, where it is used
**Reason**: The top `/` towards zero (A1, #1222) and the cut by the final direction (#1221) depend on the destination.
**Migration**: Requirements "The conversion to a whole destination rounds down and wraps" and "An operation with a Quick is computed in 48.16".

### Requirement: A constant with a point is fractional
**Reason**: The Float reading beside whole numbers and under a whole destination (#1222) is replaced by the reading by the operands.
**Migration**: Requirement "A constant with a point is read by its operands".

### Requirement: A whole-number subexpression takes the fractional format of its expression
**Reason**: A whole division is whole unless a named exception applies (beside a Quick, `q+w/2` 2; under a Quick or Float destination through `+ - *`); `l1/3>3.2` is false.
**Migration**: Requirements "A whole division rounds towards zero", "A whole division beside a Quick keeps its fraction", "The whole-division exception under a Quick or Float destination" and "Comparisons compare the full value".

### Requirement: 16.16 operations give today's values
**Reason**: Quick operations are computed in 48.16; qmul's and qdivide's 16.16 values no longer occur.
**Migration**: Requirement "An operation with a Quick is computed in 48.16".
