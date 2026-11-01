# Spec Delta

## ADDED Requirements

### Requirement: A Function may be called as a statement
In AB3 mode a call `name{…}` of a `Function` at the start of a statement SHALL compile like a call of a `Statement`: the function is called with its parameters and its result is discarded, as in AmiBlitz3; in BB2 mode it SHALL stay the error "Illegal Procedure Call".

#### Scenario: Numeric function as a statement
- **WHEN** an AB3 program defines `Function.l add{a.l,b.l}` that prints its sum and returns it, and has the line `add{2,3}`
- **THEN** it compiles and prints 5

#### Scenario: String function as a statement
- **WHEN** an AB3 program calls a `Function.s` that prints a text as a statement, followed by `NPrint "next"`
- **THEN** it compiles, prints the text and then `next`, as AmiBlitz3's executable does

#### Scenario: BB2 mode
- **WHEN** a BB2-mode program has the line `add{2,3}` with `add` a `Function`
- **THEN** the compile fails with "Illegal Procedure Call", as with `blitz2`
