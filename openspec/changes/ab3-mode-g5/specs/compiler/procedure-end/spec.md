# Spec Delta

## Purpose

Defines which `End` and `Return` words may close or leave which kind of
procedure (`Statement` or `Function`), in BB2 and in AB3 mode, including
the one case where a mismatch is accepted with a warning.

## ADDED Requirements

### Requirement: End and Return must match the open procedure
`End Function` SHALL close only a `Function`, `End Statement` only a `Statement`, `Function Return` SHALL leave only a `Function` and `Statement Return` only a `Statement`. A mismatch SHALL be refused: "Illegal End Procedure" for an `End`, "Illegal Procedure return" for a `Return`, except as the next requirement says. Outside a procedure `End Function`, `End Statement` and `Function Return` SHALL give "Illegal End Procedure". This holds in both modes (rule of #940).

#### Scenario: End Function in a Statement
- **WHEN** a `Statement` is closed with `End Function`
- **THEN** the compile fails with "Illegal End Procedure"

#### Scenario: Statement Return in a Function
- **WHEN** a `Function.l` contains `Statement Return`
- **THEN** the compile fails with "Illegal Procedure return"

### Requirement: End Statement may close a numeric Function with a warning
`End Statement` closing a `Function` whose type is byte, word, long, quick or float (and, in AB3 mode, double) SHALL be accepted and compiled exactly as `End Function` is, so that the function returns its default return value (the value `End Function` returns), and SHALL give the warning `procend`. The program SHALL work as it does with `End Function`. This holds in BB2 and in AB3 mode.

#### Scenario: Numeric Function ended with End Statement
- **WHEN** `Function.w gendart{...}` is closed with `End Statement`
- **THEN** the compile succeeds with the warning `procend`, and the executable is byte-identical to that of the source with `End Function`

#### Scenario: CD sources
- **WHEN** `EXAMPLES/BLITZMODE/DARTS_POLY.BB` and `DARTS_WIRE.BB` of the CD are compiled
- **THEN** they compile with the warning, as the original compiled them without one

#### Scenario: Warning as error
- **WHEN** the warning `procend` is in the error set
- **THEN** the same source is refused with the message of the warning

### Requirement: A string Function ended with End Statement stays an error
`End Statement` closing a `Function` whose type is string (`Function$`) or a NewType (`Function.nt`) SHALL be refused with "Illegal End Procedure". A string `Function` left with `Statement Return` SHALL stay "Illegal Procedure return".

#### Scenario: String Function
- **WHEN** `Function$ f{}` is closed with `End Statement`
- **THEN** the compile fails with "Illegal End Procedure" and no executable is written

### Requirement: The other mismatches stay refused
`End Function` closing a `Statement`, `Statement Return` in a `Function` and `Function Return` in a `Statement` SHALL stay errors as above; the relaxation covers only `End Statement` closing a numeric `Function`.

#### Scenario: Function Return in a Statement
- **WHEN** a `Statement` contains `Function Return 1`
- **THEN** the compile fails with "Illegal Procedure return"
