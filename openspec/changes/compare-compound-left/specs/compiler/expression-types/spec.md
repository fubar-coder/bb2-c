# Spec Delta

## ADDED Requirements

### Requirement: The left operand of a comparison is compared with its own value whatever its shape
The left operand of a comparison SHALL be evaluated in its own type, not in the type the expression is wanted in, also when it is an expression, a negation or a constant, and the comparison SHALL be exact for the two operand types.

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

### Requirement: Comparisons with a whole-number compound left operand keep their results
A comparison whose left operand contains only whole-number operands SHALL give the boolean it gave before.

#### Scenario: Words on the left
- **WHEN** a program evaluates `r.l=(w+v>6)` with `w=3`, `v=4`
- **THEN** r is -1 as before
