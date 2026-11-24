# Spec Delta

## ADDED Requirements

### Requirement: The value of an operation does not depend on the order of its operands
For a commutative operator the compiler SHALL give the same value for `a op b` and `b op a`, and the same value for a subexpression wherever it stands in a chain of operators of one precedence.

#### Scenario: Base plus scaled Quick
- **WHEN** a program evaluates `adr.l=base+q*4` and `adr2.l=q*4+base`
- **THEN** adr and adr2 are equal

#### Scenario: Chain of Words and a Quick under a Word destination
- **WHEN** a program evaluates `x.w=w+v+q*2` and `y.w=q*2+w+v` with `q=1.5`, `w=3`, `v=4`
- **THEN** x and y are equal

#### Scenario: Comparison of a chain
- **WHEN** a program evaluates `If l+q*2>12` and `If q*2+l>12` with `l=10`, `q=1.5`
- **THEN** both conditions have the same outcome

### Requirement: The left operand of a comparison is compared with its own value whatever its shape
The left operand of a comparison SHALL be typed by its own operands, not by the type the expression is wanted in, also when it is an expression, a negation or a constant, and the comparison SHALL be exact for the two operand types.

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
- **THEN** rf is -1

### Requirement: A Quick keeps its fraction when nothing asks for a whole number
When no destination, parameter or operand asks for a whole-number type, an expression with a Quick operand SHALL give the Quick value with its fraction.

#### Scenario: Constants plus a Quick in Print
- **WHEN** a program executes `NPrint 1+1+1+q` with `q=1.5`
- **THEN** it prints 4.5

## REMOVED Requirements

### Requirement: Known limits of the left operand until the expression tree
**Reason**: the limit (#1183) ends with the operand typing: the left operand of a comparison is typed by its own operands (requirement "The left operand of a comparison is compared with its own value whatever its shape").
**Migration**: none; `-q` on the left keeps its scenario in the new requirement.

## MODIFIED Requirements

### Requirement: No other result changes
A program that has no Quick operand in an arithmetic expression under a whole-number type so far, and a program whose Quick operands in such expressions hold whole numbers, SHALL give the same results as before the change. Comparisons (#1177) and array indexes (#1179) are not changed by this requirement.

#### Scenario: Sources unchanged
- **WHEN** the corpus and the CD's sources that compile are compiled and run before and after
- **THEN** the outputs are the same except for the places whose result differs today, listed with the change; every other executable that differs is named for the comparison with the previous commit
