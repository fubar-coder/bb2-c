# Spec Delta

## MODIFIED Requirements

### Requirement: Line joining with @@
In AB3 mode `@@` at a parameter boundary (start of a parameter list, after a parameter, after `,`; also in macro text and definitions and in multiple assignments) SHALL join the next line, the rest of the line after `@@` being ignored; strings and comments SHALL not be affected. `@@` at other places of an expression, for example after an operator, SHALL join the next line as well (AmiBlitz3: syntax error; intended deviation). Inside an include only `,` followed by `@@` SHALL be allowed, other forms giving "Joining lines with @@ does not work in includes".

#### Scenario: Tag list over lines
- **WHEN** a statement's parameters are split with `,@@` over three lines
- **THEN** it compiles as if written on one line

#### Scenario: @@ in a string
- **WHEN** a string literal contains `@@`
- **THEN** the string keeps the two characters

#### Scenario: Join after an operator
- **WHEN** a program has `b.l = 1 + @@` and `2` on the next line
- **THEN** it compiles as `b.l = 1 + 2` and `b` is 3
