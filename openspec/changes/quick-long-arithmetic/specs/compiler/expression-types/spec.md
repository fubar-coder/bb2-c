# compiler/expression-types Specification

## Purpose
Says what an arithmetic operator gives when one operand is a Quick and the
other operand or the type the expression is wanted in is a whole-number
type, and which parts of the typing stay as in the original.

## ADDED Requirements

### Requirement: A Quick operand is computed with its fraction
When an arithmetic operator (`+`, `-`, `*`, `/`) has a Quick operand and the other operand or the type so far is a Byte, Word or Long, the compiler SHALL compute the operation with the Quick's value, fraction included, and SHALL round the result once, at the end of the operation, when it must give a whole-number type: down for `+`, `-` and `*`, towards zero for `/` as BB2's Long division does. It SHALL NOT cut the Quick when it is fetched.

#### Scenario: Long destination
- **WHEN** a program evaluates `r.l=q*2`, `r.l=l-q`, `r.l=l*q`, `r.l=q/h`, `r.l=l+q` with `q=1.5`, `h=0.5`, `l=10`
- **THEN** it gets 3, 8, 15, 3 and 11

#### Scenario: Negative values
- **WHEN** a program evaluates `r.l=n*2`, `r.l=l*n`, `r.l=m/n` and `r.l=l/n` with `n=-1.5`, `l=10`, `m=9`
- **THEN** it gets -3, -15, -6 and -6 (the inexact quotient rounds towards zero)

#### Scenario: Whole-number Quick
- **WHEN** the Quick operand has no fraction (`q=3.0`)
- **THEN** every result of an arithmetic operator equals the result of the previous compiler, as long as no intermediate value overflows

### Requirement: Rounding happens once per operation, to a whole-number type
The compiler SHALL round down only when an operation gives a Byte, Word or Long, or when the value is stored in or passed as a whole-number type. A Quick OP Quick, or a Quick expression stored in a Quick, SHALL keep the fraction. A Float operand SHALL make the operation a Float as before. A chain SHALL be rounded after each operation that gives a whole-number type, not computed exactly: `r.l=l+q+p` with `l=10`, `q=1.5`, `p=2.75` gives 13 (exactly 14).

#### Scenario: Quick destination keeps the fraction
- **WHEN** a program evaluates `t.q=w+v+q` with `w=3`, `v=4`, `q=1.5`
- **THEN** `t` is 8.5

#### Scenario: Chain
- **WHEN** a program evaluates `r.l=l+q+p` with `l=10`, `q=1.5`, `p=2.75`
- **THEN** `r` is 13

### Requirement: The expected type flows as before
The type the expression is wanted in (the destination, the parameter type, Long for an array index), a Long operand to the left, and the types Word+Word, Word*Word and a shift give SHALL decide the type of the operations as before; a constant SHALL take that type. The order dependence of the previous compiler SHALL stay: `x.w=w+v+q*2` and `x.w=q*2+w+v` MAY still compute in different types. Only the cut of a Quick when it is fetched goes away. The operands of a comparison are the exception: they do not take the type (requirement "The operands of a comparison are compared with their own values", change `quick-compare-types`, #1177).

#### Scenario: Word and Long contexts
- **WHEN** a program prints `NPrint 1+1+1+q` and `NPrint w+v+q` with `q=1.5`, `w=3`, `v=4`
- **THEN** it prints 4.5 and 8.5

#### Scenario: Address idiom
- **WHEN** a program evaluates `adr.l=base+i*4` with `i` a default-typed variable holding 20000 and `base` a Long
- **THEN** the result is `base+80000`, as before (the Long operand to the left keeps the computation in Long)

### Requirement: The rule is the same in both compiler modes
blitz2 and blitz2-ab3 (AB3 mode) SHALL apply the same rule; AmiBlitz3 v3.14.0 keeps the old one.

#### Scenario: AB3 mode
- **WHEN** an AB3 source evaluates `r.l=q*2` with `q=1.5`
- **THEN** `r` is 3

### Requirement: A mixed product beyond 32 bits wraps
A Long times a Quick whose result does not fit a Long SHALL give the result modulo 2^32, as the Long multiplication does.

#### Scenario: Overflow
- **WHEN** a program evaluates `r.l=l*q` with `l=100000`, `q=30000.5`
- **THEN** `r` is the exact product modulo 2^32

### Requirement: No other result changes
A program that has no Quick operand in an arithmetic expression, and a program whose Quick operands in such expressions hold whole numbers, SHALL give the same results as before the change. Comparisons (#1177) and array indexes (#1179) are not changed by this requirement.

#### Scenario: Sources unchanged
- **WHEN** the corpus and the CD's sources that compile are compiled and run before and after
- **THEN** the outputs are the same except for the places whose result differs today, listed with the change; every other executable that differs is named for the comparison with the previous commit
