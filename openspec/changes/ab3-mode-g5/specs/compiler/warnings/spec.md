# Spec Delta

## ADDED Requirements

### Requirement: A numeric Function ended with End Statement warns
`End Statement` closing a `Function` of a numeric type SHALL give the warning `procend` with the text "End Statement closes a Function: compiled as End Function" and the source line of the `End Statement`; it SHALL be enabled by default, and in the error set it SHALL be refused as `compiler/procedure-end` says. The compile and the executable SHALL be those of `End Function`.

#### Scenario: Command line
- **WHEN** a source with such a Function is compiled with `blitz2-ab3 -s prog.ab3 -e RAM:prog`
- **THEN** the console shows `prog.ab3:LINE: warning: ... [procend]`, the executable is written and the return code is 5

#### Scenario: Disabled
- **WHEN** the warning is disabled
- **THEN** the compile is silent and the executable is the same
