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

### Requirement: A Quick keeps its fraction when nothing asks for a whole number
When no destination, parameter or operand asks for a whole-number type, an expression with a Quick operand SHALL give the Quick value with its fraction.

#### Scenario: Constants plus a Quick in Print
- **WHEN** a program executes `NPrint 1+1+1+q` with `q=1.5`
- **THEN** it prints 4.5
