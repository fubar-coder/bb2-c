# Spec Delta

## Purpose

Says how the compiler types an expression: what an operator gives for its operands' types and the type the expression is wanted in, and which parts of the typing stay as in the original.

## ADDED Requirements

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
