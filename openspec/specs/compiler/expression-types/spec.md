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
Until the expression tree (#1180), a left operand of a comparison that is an expression (`q*2`, `-q`) SHALL still be evaluated in the type the expression is wanted in (#1183), and a Long constant on the left under a Float context SHALL still be compared as a Float.

#### Scenario: Compound left operand
- **WHEN** a program evaluates `r.l=(q*2>2)` with `q=1.5` (the left operand is an expression)
- **THEN** it gives 0 as before; `rw.w=(q*2>2)` gives -1

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
