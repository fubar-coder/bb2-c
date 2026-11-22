# Spec Delta

## Purpose

Says how the compiler types an expression: what an operator gives for its operands' types and the type the expression is wanted in, and which parts of the typing stay as in the original.

## ADDED Requirements

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

### Requirement: A whole-valued literal stays whole under a Byte type
When the type so far is Byte and a literal written with a point or an exponent has a whole value (`2.0`, `1e2`), the compiler SHALL take it as the whole-number constant it is, as before.

#### Scenario: Whole literal with a point
- **WHEN** a program evaluates `rb.b=vb+2.0` and `rb.b=vb AND 2.0` with `vb=3`
- **THEN** it gets 5 and 2, and both compile as before

### Requirement: Byte data keeps its conversion
`Data.b` with a literal that has a fraction SHALL give the literal's whole part, as before.

#### Scenario: Data.b
- **WHEN** a program reads `Data.b 1.5` into a Byte
- **THEN** it gets 1

### Requirement: A fraction literal with a bit operator under a Byte type is refused
When the type so far is Byte and a literal with a non-zero fraction is an operand of `AND`, `OR`, `EOR`, a shift or `NOT`, the compiler SHALL refuse the expression with "Illegal Operator for Type", as under a Word type so far.

#### Scenario: AND with a fraction literal
- **WHEN** a program contains `rb.b=1.5 AND vb` or `rb.b=NOT 1.5`
- **THEN** compiling it gives "Illegal Operator for Type"
