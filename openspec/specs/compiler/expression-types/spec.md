# compiler/expression-types Specification

## Purpose
Says what the compiler's typing of operators gives; this change covers the comparison operators: which type their operands are compared in, and which type the boolean has.

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
- **THEN** the outputs are the same; every executable that differs is named for the comparison with the previous commit

#### Scenario: Compound right operand in its own types (accepted until #1180)
- **WHEN** a program evaluates `If base<i*4+base` with `base.l=$40000` and a default-typed (Quick) `i=20000`
- **THEN** `i*4` is computed as a Quick (it overflows) and the condition is false; before, the right operand was computed in Long and the condition was true

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
When a Quick is followed by a Long operand that makes the type so far Long by itself (a constant beyond the Quick range, `&var`, `?label`) and no type was set before or the type so far is Quick (a Quick destination), `*` and `/` SHALL be computed with the Quick's fraction as in "A Quick operand is computed with its fraction"; `+`, `-` and the other operators SHALL be a Long operation on the Quick rounded down, as before. `Int(q)` gives the previous result for `*` and `/`.

#### Scenario: Multiplication by a large constant
- **WHEN** a program prints `NPrint q*100000` and `NPrint Int(q)*100000` with `q=1.5`
- **THEN** it prints 150000 and 100000

#### Scenario: Address operand
- **WHEN** a program prints `NPrint q+&l-&l` with `q=1.5`
- **THEN** it prints 1, as before

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

### Requirement: A bit operator with a fraction constant in a later index is refused
A later index whose expression applies `AND`, `OR`, `EOR` or a shift to a Quick and a constant with a fraction SHALL be refused with "Illegal Operator for Type" with and without runtime error checking, as the first index and the checked build refuse it.

#### Scenario: Quick AND fraction constant
- **WHEN** a program compiled without runtime error checking contains `ja(1,vq AND 1.5)` with `vq.q=1.5`
- **THEN** compiling it gives "Illegal Operator for Type"

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
