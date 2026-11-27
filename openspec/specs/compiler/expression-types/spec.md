# compiler/expression-types Specification

## Purpose
Says what the compiler's typing of operators and operands gives: comparisons (the type their operands are compared in, the type of the boolean, a comparison as an operand), arithmetic with a Quick operand under a whole-number type, the bit operators on a Quick, negative whole constants under a Quick type, array indexes, constants with a fraction under a Byte type, and `^` with a negative base.

## Requirements

### Requirement: The operands of a comparison are compared with their own values
The right operand and a left operand that is a single element (a variable, field, array element, function value, `&var` or constant) of `=`, `<>`, `<`, `<=`, `>`, `>=` SHALL NOT take the type the expression is wanted in (a destination, a parameter type, a type so far from the left): each operand keeps its own type, and the two operands are compared in the common type of the two. Only the boolean SHALL take the type of the context. A Quick SHALL NOT be cut to a Long; two whole numbers SHALL be compared as whole numbers.

#### Scenario: Long destination
- **WHEN** a program evaluates `r.l=(q>1)`, `r.l=(q=1)`, `r.l=(q>1.4)` and `r.l=(1.4<q)` with `q=1.5`
- **THEN** it gets -1, 0, -1 and -1

#### Scenario: Long parameter
- **WHEN** a program prints `Hex$(q>1)` with `q=1.5`
- **THEN** it prints FFFFFFFF

#### Scenario: Word and Byte destinations
- **WHEN** the destination is a Word or a Byte (`rw.w=(q>1)`)
- **THEN** the result is as before (-1)

#### Scenario: Quick destination
- **WHEN** a program evaluates `rq.q=(lw>1)` and `rq.q=(lw=1)` with `lw.l=65537`
- **THEN** it gets -1 and 0

#### Scenario: Float destination and parameter
- **WHEN** a program evaluates `rf.f=(l2=l3)`, `rf.f=(l2>l3)`, `rf.f=(l3<l2)` and `rf.f=tf{l2>l3}` (a Float parameter) with `l2.l=100000001`, `l3.l=100000000`
- **THEN** it gets 0, -1, -1 and -1

### Requirement: A Long or Word against a Quick is compared exactly
A comparison of a Byte, Word or Long with a Quick, in either order, SHALL compare the whole-number operand with `floor(q)` (the high word of the Quick, sign-extended) and, when they are equal, SHALL let the fraction (the low word) decide. A Long beyond +-32768 SHALL NOT be converted to a Quick. The two orders of the same comparison SHALL give the same result.

#### Scenario: Both orders
- **WHEN** a program evaluates `If q>l` and `If l<q` with `q=1.5`, `l=1`
- **THEN** both are true

#### Scenario: Equality decides by the fraction
- **WHEN** a program compares `l=q`, `l<q`, `l>q` with `l=1`, `q=1.5` and with `l=1`, `q=1.0`
- **THEN** the first is false, true, false; the second is true, false, false

#### Scenario: Negative and large values
- **WHEN** a program compares `l>n` and `n<l` with `n=-1.5` and `l=-1`, and `l=q` with `l=100000`, `q=1.5`
- **THEN** `l>n` and `n<l` are true (-1 is greater than -1.5), `l=q` is false

### Requirement: A comparison gives a boolean in the type of the context
The result of a comparison SHALL be true (-1) or false (0) in the type of the destination or parameter; the operands after a comparison start with the context type again (`If l=5 OR q*2>3` as before).

#### Scenario: Comparison inside arithmetic
- **WHEN** a program evaluates `r.l=l+(q>1)` with `q=1.5`, `l=10`
- **THEN** `r` is 9

### Requirement: Comparisons follow the same rule in both compiler modes
blitz2 and blitz2-ab3 (AB3 mode) SHALL apply the same rule for comparisons; AmiBlitz3 v3.14.0 keeps the old one.

#### Scenario: AB3 mode
- **WHEN** an AB3 source evaluates `r.l=(q>1)` with `q=1.5`
- **THEN** `r` is -1

### Requirement: Comparisons that were right keep their results
A comparison of whole numbers, of two Quicks, of a Float with anything, and of strings SHALL give the same result as before, including the byte-constant rule (an equality comparison of a Byte with a constant from 128 to 255). A comparison whose operands hold whole numbers in a Quick SHALL give the same boolean as before, as long as no intermediate value overflows.

#### Scenario: Sources unchanged
- **WHEN** the corpus and the CD's sources that compile are compiled and run before and after
- **THEN** the outputs are the same except for the places the change lists; every executable that differs is named for the comparison with the previous commit

#### Scenario: Compound right operand in its own types (accepted until #1180)
- **WHEN** a program evaluates `If base<i*4+base` with `base.l=$40000` and a default-typed (Quick) `i=20000`
- **THEN** with the operand typing (#1180) the condition is true: the Long takes part, so `i*4+base` is computed in 48.16 (before the change the Quick product overflowed and the condition was false)

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

### Requirement: Negation and conversion round down
Unary minus of a Quick under a whole-number type so far SHALL negate the value with its fraction and round it down; the conversion of a Quick to a Byte, Word or Long (an assignment, a parameter, the end of an expression) SHALL round down as before. `Int(q)` SHALL truncate towards zero as before.

#### Scenario: Negation
- **WHEN** a program evaluates `r.l=-q`, `r.l=0-q`, `x.w=-q` and `r.l=-q*2` with `q=1.5`
- **THEN** it gets -2, -2, -2 and -3

#### Scenario: Conversion
- **WHEN** a program evaluates `r.l=n` and `r.l=Int(n)` with `n=-1.5`
- **THEN** it gets -2 and -1

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
A program that has no Quick operand in an arithmetic expression under a whole-number type so far, and a program whose Quick operands in such expressions hold whole numbers, SHALL give the same results as before the change, except where an intermediate value overflowed before and is now computed in a wider format, and except the cases the requirements of this change name. Array indexes (#1179) are not changed by this requirement.

#### Scenario: Sources unchanged
- **WHEN** the corpus and the CD's sources that compile are compiled and run before and after
- **THEN** the outputs are the same except for the places the change lists (the named matrix cells, the CD places of the prev run); every other executable that differs is named for the comparison with the previous commit

### Requirement: Every array index is evaluated as a Long
The compiler SHALL evaluate every index of an array (the first and every later one) as a Long expression, with and without runtime error checking, by the rules of a Long evaluation (`quick-long-arithmetic`: a Quick operand keeps its fraction until the operation). An index SHALL address the same element with and without runtime error checking whenever the checked build does not report it.

#### Scenario: Quick calculation beyond the Quick range
- **WHEN** a program with `Dim g.b(1,60000),h.b(60000,1)` and a default-typed `y=200` reads `g(1,y*200/2)` and `h(y*200/2,1)`, compiled without and with runtime error checking
- **THEN** both builds read `g(1,20000)` and `h(20000,1)`

#### Scenario: Shift of a Quick in a later index
- **WHEN** a program reads `b(1,q ASL 4)` with `q=1.5`, compiled without runtime error checking
- **THEN** it reads `b(1,16)`, as the checked build does

#### Scenario: Quick with a fraction keeps its value
- **WHEN** a program reads `b(1,q*10)`, `c(1,5,q*10)` and assigns `v(1,q*10)=99` with `q=1.5`, compiled without and with runtime error checking
- **THEN** both builds use element 15 of the later index

#### Scenario: Large whole-number indexes
- **WHEN** a program with `Dim g.b(1,60000)` reads `g(1,40000)`, `g(1,l)` with `l.l=40000` and `g(1,wv*2)` with `wv.w=20000`, without and with runtime error checking
- **THEN** both builds read `g(1,40000)`, as before

### Requirement: Without runtime error checking an index keeps its low word
Without runtime error checking the compiler SHALL NOT check the range of an index; an index outside 0..65535 SHALL use its low word, as before. With runtime error checking an index outside 0..65535 or beyond its dimension SHALL give "Array Subscript out of Range", as before.

#### Scenario: Index beyond 65535 without the checking
- **WHEN** a program with `Dim g.b(1,60000)` and `l4.l=40000` reads `g(1,l4*2)` without runtime error checking
- **THEN** it reads the element at index 14464 (80000 modulo 65536), as before

#### Scenario: Index beyond 65535 with the checking
- **WHEN** the same program is compiled with runtime error checking
- **THEN** it stops with "Array Subscript out of Range"

### Requirement: The index code of a plain later index does not grow
Without runtime error checking, a later index that is one Byte, Word, Quick or Long variable or a constant SHALL compile to code of the same size as before this change, and a later index whose `*` or `/` has two Byte or Word operands (variables or constants up to 32767) SHALL call no library routine for it, in both builds. Its value SHALL be the Long operation's value, also for a Word -32768 divided by -1. A divisor 0 SHALL trap in the CPU as a Word division does: with runtime error checking the program stops with "Division by Zero", as the Long division reports it; without it the trap is not caught, as before.

#### Scenario: Two Word variables
- **WHEN** a program compiled without runtime error checking reads `a(iw,jw)` with Word variables
- **THEN** the code of the access has the same size as before

#### Scenario: Word multiplication in a later index
- **WHEN** a program reads `a(iw,jw*kw)` with `jw.w=300`, `kw.w=200` and `Dim a.b(1,60000)`
- **THEN** the access calls no library routine for the product and reads element 60000

#### Scenario: Word division overflow
- **WHEN** a program with runtime error checking and `Dim g.b(1,40000)` reads `g(1,wv/wm)` with `wv.w=-32768` and `wm.w=-1`
- **THEN** it reads `g(1,32768)` without a runtime error, as before

#### Scenario: Word division by 0 with the checking
- **WHEN** a program with runtime error checking and `Dim g.b(1,100)` reads `g(1,wv/wz)` with `wv.w=5` and `wz.w=0`
- **THEN** it stops with "Division by Zero", as before

### Requirement: A comparison as right operand keeps the left operand's type
When the right operand of an operator is a comparison (also in brackets, bound by precedence, nested in a bracket, or after `NOT` or unary minus), the compiler SHALL convert the comparison's boolean to the left operand's type and compute the operation in that type. The left operand SHALL keep its type and value.

#### Scenario: Long left operand
- **WHEN** a program evaluates `rl.l=lw AND (l1=l1)`, `rl.l=lw AND l1=l1`, `rl.l=lw OR (l1<>l1)` and `rl.l=lb+(l1=l1)` with `lw=65537`, `lb=100000`, `l1=1`
- **THEN** it gets 65537, 65537, 65537 and 99999

#### Scenario: Condition without a destination
- **WHEN** a program evaluates `If $10000 AND (l1=l1)` with `l1=1`
- **THEN** the condition is true

#### Scenario: Float and Quick left operands
- **WHEN** a program evaluates `rf.f=fh+(l1=l1)`, `rf.f=f25*(l1=l1)`, `rf.f=fh+(0+(l1=l1))` and `rq.q=q+(l1=l1)` with `fh=0.5`, `f25=2.5`, `q=1.5`, `l1=1`
- **THEN** it gets -0.5, -2.5, -0.5 and 0.5

#### Scenario: Unchanged cases
- **WHEN** a program evaluates `rl.l=(l1=l1) AND lw`, `rf.f=w+(l1=l1)` and `rf.f=fh+tf{l1=l1}` (a function with a Float parameter returning it)
- **THEN** it gets 65537, 2 and -0.5, as before

### Requirement: A Float left operand of a bit operator with a comparison is refused
When the left operand is a Float and the operator is `AND`, `OR`, `EOR` or a shift and its right operand is a comparison, the compiler SHALL refuse the expression with "Illegal Operator for Type", as for any other Float operand of these operators.

#### Scenario: Float AND comparison
- **WHEN** a program contains `rf.f=fh AND (l1=l1)`
- **THEN** compiling it gives "Illegal Operator for Type"

### Requirement: A fraction literal keeps its fraction under a Byte type
When the type so far is Byte and a decimal literal has a non-zero fraction, the compiler SHALL take the literal as a Float constant, compute the operation in Float and convert the result to the Byte at the end (towards zero), as under a Word type so far.

#### Scenario: Byte destination
- **WHEN** a program evaluates `rb.b=(1.5+1.5)`, `rb=(0.5*4)`, `rb=(1.5*vb)` and `rb=(2.5*2)` with `vb=2`
- **THEN** it gets 3, 2, 3 and 5

#### Scenario: Same as under Word
- **WHEN** a program evaluates the same expressions into `rw.w`
- **THEN** it gets the same values as into `rb.b`

#### Scenario: A literal alone
- **WHEN** a program evaluates `rb.b=1.5` and `rb.b=-1.5`
- **THEN** it gets 1 and -1, as before

### Requirement: Byte data keeps its conversion
`Data.b` with a literal that has a fraction SHALL give the literal's whole part, as before.

#### Scenario: Data.b
- **WHEN** a program reads `Data.b 1.5` into a Byte
- **THEN** it gets 1

### Requirement: A negative base with a whole exponent keeps the sign of the power
In BB2 mode `x^y` with a negative `x` and a `y` whose value is a whole number SHALL give the power's magnitude as before, negative when `y` is odd and positive when `y` is even. This SHALL hold for every operand type and every destination.

#### Scenario: Odd exponent
- **WHEN** a program evaluates `rl.l=vf^nw` and `rf.f=vf^nw` with `vf=-2.5`, `nw=3`
- **THEN** it gets -15 and -15.625

#### Scenario: Even and negative exponents
- **WHEN** a program evaluates `rf.f=vm^2` and `rf.f=vm^-1` with `vm=-2`
- **THEN** it gets 4 and -0.5

#### Scenario: Whole exponent of a Float or Quick type
- **WHEN** a program evaluates `rf.f=vf^e` with `vf=-2.5` and `e.f=3` or `e.q=3`
- **THEN** it gets -15.625

#### Scenario: Positive base unchanged
- **WHEN** a program evaluates `rf.f=a^b` with `a=2.5` and any `b`
- **THEN** it gets the same value as before

### Requirement: A negative base with a fractional exponent stays as before
`x^y` with a negative `x` and a `y` with a fraction SHALL give the same value as before (the magnitude |x|^y).

#### Scenario: Fractional exponent
- **WHEN** a program evaluates `rl.l=(-3)^3.5`
- **THEN** it gets 46, as before

### Requirement: AB3 mode keeps AmiBlitz3's power
In AB3 mode `^` SHALL give the result of AmiBlitz3's math library, also for a negative base.

#### Scenario: AB3 mode
- **WHEN** an AB3-mode program evaluates `rf.f=vf^3` with `vf=-2.5`
- **THEN** it gets AmiBlitz3's result (15.625 with v3.14.0)

### Requirement: NOT, AND, OR and EOR under a Quick type take the whole part
`NOT`, `AND`, `OR`, `EOR`, `&` and `|` under a Quick type so far (a Quick or default-typed variable as an operand, a Quick destination or parameter) SHALL take the whole part of a Quick operand, its 16 fraction bits dropped and rounded down (`-1.5` is `-2`), and give a Quick. The shifts SHALL keep the 16.16 value and its fraction. Whole operands give the same values as before.

#### Scenario: Toggle of a default-typed flag
- **WHEN** a program evaluates `h=True : h=NOT h`, `h=-1 : h=NOT h`, `h=(l=5) : h=NOT h`, `h=On : h=NOT h` and `h=0 : h=NOT h`
- **THEN** it gets 0, 0, 0, 0 and -1

#### Scenario: NOT
- **WHEN** a program evaluates `h.q=NOT x` with `x=3`, `h=NOT 3`, `h=NOT 1.5`, `h=NOT -1.5` and `h=NOT q` with `q=1.5`
- **THEN** it gets -4, -4, -2, 1 and -2

#### Scenario: AND, OR and EOR of Quick operands
- **WHEN** a program evaluates `q AND p`, `q OR p`, `q EOR w`, `p & q` and `q | p` with `q=1.5`, `p=2.5`, `w.w=3`
- **THEN** it gets 0, 3, 2, 0 and 3

#### Scenario: Negative Quick operand and whole masks
- **WHEN** a program evaluates `q AND w` with `q=-1.5`, `w=3`, `z AND 6` with `z.q=13` and `y OR 8` with `y=7`
- **THEN** it gets 2, 4 and 15

#### Scenario: Truth test on a fraction
- **WHEN** a program evaluates `If h OR g` with `g=0` and `If h AND g` with `h=0.5`, `g=0.5`
- **THEN** `If h OR g` is false for `h=0.5` and true for `h=-0.5`, and `If h AND g` is false

#### Scenario: Shifts keep the fraction
- **WHEN** a program evaluates `x LSR 1` with `x=5`, `q LSL 1` and `q ASR 1` with `q=1.5` under a Quick destination
- **THEN** it gets 2.5, 3 and 0.75

### Requirement: A negative whole constant under a Quick type is the Quick of its value
A whole-number constant read under a Quick type so far (`On`, a `#` constant, a number in a sum or product) SHALL be the Quick whose integer part is the value and whose fraction is 0, for a negative value as for a positive one. A number outside -32768..32767 SHALL stay a Long. A minus sign before a literal or constant is not affected. A default-typed flag set to `On` and toggled with `NOT` SHALL give the same truth values as before.

#### Scenario: Negative constants
- **WHEN** a program evaluates `q.q=On`, `q=#c` with `#c=-3`, `q=1+On`, `q=0.5+#c`, `q=#c*0.5` and `q=#e` with `#e=-32768`
- **THEN** it gets -1, -3, 0, -2.5, -1.5 and -32768

#### Scenario: Unchanged
- **WHEN** a program evaluates `q=-3`, `q=-#d` with `#d=3`, `q=#g` with `#g=32767` and `q=0.25+#d`
- **THEN** it gets -3, -3, 32767 and 3.25

#### Scenario: Flag toggle with On
- **WHEN** a program sets `flag=On` and then evaluates `flag=NOT flag`, once and twice
- **THEN** `If flag` is false after one `NOT` and true after two

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
- **WHEN** a program evaluates `r.l=(w+v>c)` and `r.l=(w-(-v)>c)` with `w=20000`, `v=20000`, `c=0`, `r.l=(q*1000>50000)` with `q=100`, and `rb.b=(b1+b2>150)` with `b1.b=100`, `b2.b=100`
- **THEN** r is -1 and -1, r is -1 and rb is -1

#### Scenario: A whole-number sum on the left under a Word type is compared as a Long
- **WHEN** a program evaluates `rw.w=(w+v>c)` and `If w+v>c` with `w=20000`, `v=20000`, `c=0`
- **THEN** rw is -1 and the condition is true (the comparison's common format is at least Long; today and in the original the Word sum overflows: 0 and false)

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
A Quick expression's result SHALL be rounded once, where it is used: down to a whole number, the 16.16 value for a Quick, without a wanted type the Quick when it fits, else rounded down. A `/` under a wanted Long SHALL be whole, towards zero, at the operator, and a `/` at the top under a wanted Byte or Word towards zero (#1222). A 48.16 product or quotient SHALL be cut to 1/65536 in the direction of the final rounding (down under a wanted Byte, Word or Long, else towards zero) (#1221).

#### Scenario: Long destination
- **WHEN** a program evaluates `r.l=q*2`, `r.l=l-q`, `r.l=l*q`, `r.l=q/h`, `r.l=l+q`, `r.l=q+p` with `q=1.5`, `h=0.5`, `p=2.75`, `l=10`
- **THEN** it gets 3, 8, 15, 3, 11 and 4, as today

#### Scenario: The 48.16 cut under a Long destination
- **WHEN** a program evaluates `r.l=q*p` and `r.l=q*p+l` with `q=-0.5`, `p=1/65536`, `l=5`
- **THEN** it gets -1 and 4 (the exact value rounded down once; test `expr-q48-cut-down`)

#### Scenario: Division under a Long destination
- **WHEN** a program evaluates `r.l=i/2*2` with `i=7` and `r.l=q/2+0` with `q=-3`
- **THEN** it gets 6 and -1 (the quotient is whole at the operator, as in the original)

#### Scenario: Word destination
- **WHEN** a program evaluates `x.w=q/2` with `q=-3` and `x.w=l*q` with `l.l=100000`, `q=0.25`
- **THEN** it gets -1 (the `/` at the top towards zero, as under a Long) and 25000 (the Long takes part: 48.16)

#### Scenario: A division at the top under a Byte or Word destination
- **WHEN** a program evaluates `x.w=l/q`, `x.w=w/q`, `x.w=n/q`, `x.b=n/q` and `x.w=n/q+z` with `l.l=-10`, `w.w=-10`, `n=-10`, `q=3`, `z=0`
- **THEN** it gets -3, -3, -3, -3 and -4 (a deeper `/` is rounded down with the rest; test `expr-top-div-trunc`)

### Requirement: A constant with a point is fractional
A constant written with a point (`3.0`, `1.5`) SHALL be a fractional number whatever its value, also under a Byte type; a constant without a point SHALL be integral and take its smallest whole-number type (Byte, Word, Long). A fractional constant SHALL be a Float, as in the original, where it stands beside whole numbers only or a Byte, Word or Long is wanted (#1222), and a Quick where a Quick is in its expression or wanted (and the value fits).

#### Scenario: Beside whole numbers or under a wanted Long a Float
- **WHEN** a program evaluates `r.l=l*0.1` with `l=100`, `NPrint l*0.001` with `l=1000000`, `r.l=q*1.1` with `q=10` and `r.l=l*0.5` with `l=-3`
- **THEN** it gets 10, 1000, 11 and -1, as the original

#### Scenario: Under a wanted Byte or Word a Float beside a Quick
- **WHEN** a program evaluates `x.w=-1.5+z`, `x.b=z-1.5`, `x.w=w*0.5+z` and `x.w=n*0.25` with `z=0`, `w.w=-3`, `n=-10`
- **THEN** it gets -1, -1, -1 and -2, as under a Long destination (test `expr-top-div-trunc`)

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
`Mod` SHALL give the remainder by today's rule (a negative remainder has the divisor added, as `domodlong`), computed with the fractions of its operands whenever one has a fraction (a Quick, a Float, a constant with a point), also for two constants. A divisor 0 SHALL behave as today per type: a Byte, Word or Quick `Mod` raises the CPU's division trap, a Long or Float `Mod` gives the dividend.

#### Scenario: Negative dividend
- **WHEN** a program evaluates `x.l=q Mod 2` with `q=-7`
- **THEN** x is 1, as today

#### Scenario: Fraction
- **WHEN** a program evaluates `x.q=q Mod 2` with `q=7.5`
- **THEN** x is 1.5 (today 1)

#### Scenario: Divisor 0
- **WHEN** a program evaluates `r.l=l Mod z` with `l.l=-7`, `z.l=0`, and `x.w=w Mod z` with `w.w=7`, `z.w=0`
- **THEN** r is -7, and the second raises the division trap, as today

#### Scenario: A Quick divisor below 1
- **WHEN** a program evaluates `x.q=q Mod p` with `q=7.5`, `p=0.5`
- **THEN** x is 0 (today the whole part of `p`, 0, makes it trap)

#### Scenario: A constant with a point beside whole numbers
- **WHEN** a program evaluates `x.q=b Mod 1.5` and `NPrint b Mod 1.5` with `b.b=4`
- **THEN** x is 1 and it prints 1 (today 0 and 0: the whole parts)

#### Scenario: Two constants
- **WHEN** a program evaluates `x.q=5.25 Mod 2`, `x.w=5.25 Mod 2`, `x.q=-5.25 Mod 2`, `x.q=5 Mod 0.5`, `x.l=-5.25 Mod -2` and `NPrint 5.25 Mod 2`
- **THEN** x is 1.25, 1, 0.75, 0 and -4, and it prints 1.25, as with the variables, a whole destination rounding the Quick down (today the whole parts' Mod, and a crash for the divisor 0.5)

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

#### Scenario: A 48.16 operand of a bit operator or shift
- **WHEN** a program evaluates `r.l=(q+40000) AND b` with `q=1.5`, `b.b=3`, and `r.l=(l+q) LSL 1` with `l.l=100000`, `q=1.5`
- **THEN** it gets 1 and 200002: the 48.16 value's whole part as a Long, whatever its value

#### Scenario: A Long and a Quick under AND in both orders
- **WHEN** a program evaluates `If q AND l` and `If l AND q` with `l.l=65536`, `q=-1`
- **THEN** both conditions are true (today the first is false)

### Requirement: Bit operators take the whole part of a fraction
`AND`, `OR`, `EOR` and `NOT` SHALL accept a Quick operand or a constant with a point or a fraction on either side, under every wanted type and in an index, with its fraction cut (the whole part rounded down, as for a Quick under a Quick type so far). A Float variable SHALL be refused as before.

#### Scenario: Both sides
- **WHEN** a program evaluates `rb.b=b AND 1.5`, `rb.b=1.5 AND b` and `rb.b=b AND 1` with `b.b=3`
- **THEN** all three give 1 (before the change the first two were refused)

#### Scenario: Other types and constants
- **WHEN** a program evaluates `x.w=w OR 2.5` with `w.w=4`, `r.l=2.5 EOR l` with `l.l=7`, `r.l=b AND -1.5` with `b.b=7`, `NPrint b AND 2.0` with `b.b=3` and `rb.b=Not 1.5`
- **THEN** it gets 6, 5, 6, 2 and -2

#### Scenario: Constants only
- **WHEN** a program evaluates `rb.b=1.5 AND 3`, `NPrint 1.5 AND 3` and `r.l=(1.5 AND 3)+b` with `b.b=1`
- **THEN** it gets 1, 1 and 2, as `b AND 1.5` takes the fraction (before the change the first two were refused)

#### Scenario: Later index
- **WHEN** a program compiled with and without runtime error checking contains `ja(1,vq AND 1.5)` with `vq.q=3`
- **THEN** it compiles and reads `ja(1,1)` in both builds

#### Scenario: Float variable
- **WHEN** a program contains `r.l=l AND f` with `f.f=1.5`
- **THEN** compiling it gives "Illegal Operator for Type", as before

### Requirement: A shift count is a whole number
The count of `LSL`, `ASL`, `LSR` and `ASR` SHALL be a constant without a point or a variable or expression of a Byte, Word, Long or Quick type; a Quick count SHALL be its whole part, rounded down, as in the original. A constant with a point as a count SHALL be refused with "Illegal Operator for Type", whatever its value. A constant with a point as the shifted operand SHALL be taken as a Quick variable of its value (rounded down first under a wanted Long).

#### Scenario: A constant with a point as count
- **WHEN** a program contains `rb.b=b LSL 2.0`, `NPrint q LSL 2.5` or `x.w=x LSL 2.0`
- **THEN** compiling it gives "Illegal Operator for Type" (before the change the first two compiled, as in the original)

#### Scenario: A constant with a point as the shifted operand
- **WHEN** a program evaluates `rb.b=1.5 LSL 2`, `r.l=1.5 LSL 2`, `NPrint 1.5 LSR 1`, `NPrint 2.0 LSL 1` and `NPrint 1.5 LSL qh` with `qh=2.5`
- **THEN** it gets 6, 4, .75, 4 and 6 (before the change the first was refused, the original gave 4, and the third, fourth and fifth were refused)

#### Scenario: Quick variables as count
- **WHEN** a program executes `NPrint x LSL q2`, `NPrint x LSL qh` and `NPrint x LSL qn` with `x.w=3`, `q2=2.0`, `qh=2.5`, `qn=2.99`
- **THEN** it prints 12, 12 and 12, as today and the original

#### Scenario: A Quick count beside a whole or Quick left operand
- **WHEN** a program executes `NPrint x LSR qh` with `x.w=3`, `qh=2.5` and evaluates `rl.l=q LSL qh` with `q=1.5`
- **THEN** it prints .75 (the Word is shifted as a 16.16 value) and rl is 4, as today

### Requirement: 16.16 operations give today's values
Where the rule computes in 16.16, the compiler SHALL give today's values bit for bit: the products of `qmul`, the quotients of `qdivide` with its last bit (up to 1/65536 above the quotient towards zero), and for a divisor 0 the largest number with the dividend's sign ($7FFFFFFF, for a negative dividend $80000001).

#### Scenario: qdivide's last bit
- **WHEN** a program evaluates `x.q=q/p` with `q=-7.25`, `p=0.3`
- **THEN** x is $FFE7D508, as today

#### Scenario: Division by zero
- **WHEN** a program evaluates `x.q=q/p` with `p=0` and `q=1`, and with `q=-1`, and `r.l=l/q` with `l.l=-100`, `q=0`
- **THEN** x is $7FFFFFFF and $80000001, r is -2147483647, as today

### Requirement: A 48.16 division by 0 gives the largest value of the wanted type
A 48.16 division by 0 SHALL give the largest value in the format of the wanted type with the dividend's sign: a Quick $7FFFFFFF ($80000001), a Word 32767, a Byte 127, a Long or no wanted type $7FFFFFFF, as `lqdiv`. A 16.16 division by 0 stays today's (requirement "16.16 operations give today's values"), a documented limit: under a Byte destination it gives -1 where a 48.16 one gives 127.

#### Scenario: 48.16 division by zero without a wanted Long
- **WHEN** a program executes `NPrint l/q` with `l.l=-100`, `q=0`
- **THEN** it prints -2147483647, as today

#### Scenario: 48.16 division by zero under a Quick, Word or Byte destination
- **WHEN** a program evaluates `x.q=l/q`, `x.w=l/q` and `x.b=l/q` with `l.l=100`, `q=0`
- **THEN** x is $7FFFFFFF as today, 32767 and 127 (today -1 and -1, the low word and byte of $7FFFFFFF)

#### Scenario: The limit under a Byte destination
- **WHEN** a program evaluates `rb.b=q/p` with `q=1`, `p=0` and `x.b=l/q` with `l.l=100`, `q=0`
- **THEN** rb is -1 (the 16.16 division, as today) and x is 127 (the 48.16 division)
