# Spec Delta

## Purpose

Says how the compiler types an expression: what an operator gives for its operands' types and the type the expression is wanted in, and which parts of the typing stay as in the original.

## ADDED Requirements

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
