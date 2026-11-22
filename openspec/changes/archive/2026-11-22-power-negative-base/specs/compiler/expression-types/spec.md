# Spec Delta

## Purpose

Says how the compiler types an expression: what an operator gives for its operands' types and the type the expression is wanted in, and which parts of the typing stay as in the original.

## ADDED Requirements

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
